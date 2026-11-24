using System.Globalization;
using System.Text;
using System.Text.RegularExpressions;

//
// this shit is entierly vibecoded, don't try to understand it
//

/// <summary>
/// Translates Fusion's DX9 effect files (.fx: HLSL SM2/SM3 pixel shaders) to the GLSL fragment
/// shaders the runtime loads (GLSL 330 core and GLSL ES 300).
///
/// Pipeline: C preprocessor -> lexer -> parser (AST) -> type-checked GLSL emitter.
/// The emitter tracks HLSL types to insert the implicit conversions HLSL allows and GLSL rejects:
/// scalar splats, vector truncation, bool/float conversions, component-wise comparisons, vector
/// ternaries, intrinsic argument promotion, flattened brace initializers, writable uniforms, etc.
///
/// Runtime contract (SDL3GraphicsBackend::LoadShader / ApplyEffectParameters):
///   in vec2 TexCoord; out vec4 FragColor; uniform vec4 uColor (RGB coefficient + blend alpha);
///   uniform sampler2D uTexture (unit 0, object/layer image), uniform sampler2D bkd (unit 1, background);
///   fPixelWidth/fPixelHeight; one uniform per effect parameter, set by name
///   (int and bool params: glUniform1i, float: glUniform1f, color: glUniform4f).
///
/// All arithmetic is done in float (SM2/SM3 hardware does the same), so HLSL ints become floats;
/// subscripts are wrapped in int(). This keeps GLSL ES (no implicit int->float) happy.
/// A hand-written .frag in runtime/shaders/&lt;api&gt;/thirdparty always takes precedence.
/// </summary>
public static class HlslToGlsl
{
	public sealed class Result
	{
		public string Gl330 = "";
		public string Gles300 = "";
		public List<string> Warnings = [];
	}

	/// <param name="parameterTypes">Fusion parameter name -> type (0 int, 1 float, 2 color, 3 image), if known.
	/// Used to declare uniforms with the type the runtime sets them with.</param>
	public static Result Translate(string hlsl, IReadOnlyDictionary<string, int>? parameterTypes = null)
	{
		var warnings = new List<string>();
		string pre = Preprocessor.Run(hlsl, warnings);
		var toks = Lexer.Lex(pre);
		var unit = new Parser(toks).ParseUnit();
		var emitter = new Emitter(unit, parameterTypes, warnings);
		return emitter.Run();
	}

	// ============================================================== tokens

	enum TK { Ident, Number, Punct, String, Space, EOF }

	readonly record struct Tok(TK Kind, string Text, int Line = 0);

	sealed class TranslateException(string msg) : Exception(msg);

	static class Lexer
	{
		static readonly string[] MultiPunct = ["<<=", ">>=", "++", "--", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "==", "!=", "<=", ">=", "&&", "||", "<<", ">>", "::", "##"];
		static readonly Regex NumberRx = new(@"\G(0[xX][0-9a-fA-F]+[uUlL]?|(\d+\.\d*|\.\d+|\d+)([eE][+-]?\d+)?[fFhHuUlL]?)", RegexOptions.Compiled);

		public static List<Tok> Lex(string s, bool keepSpace = false)
		{
			var list = new List<Tok>();
			int i = 0, line = 1;
			while (i < s.Length)
			{
				char c = s[i];
				if (char.IsWhiteSpace(c))
				{
					int st = i;
					while (i < s.Length && char.IsWhiteSpace(s[i])) { if (s[i] == '\n') line++; i++; }
					if (keepSpace) list.Add(new Tok(TK.Space, s[st..i], line));
					continue;
				}
				if (char.IsLetter(c) || c == '_')
				{
					int st = i;
					while (i < s.Length && (char.IsLetterOrDigit(s[i]) || s[i] == '_')) i++;
					list.Add(new Tok(TK.Ident, s[st..i], line));
					continue;
				}
				if (char.IsDigit(c) || (c == '.' && i + 1 < s.Length && char.IsDigit(s[i + 1])))
				{
					var m = NumberRx.Match(s, i);
					list.Add(new Tok(TK.Number, m.Value, line));
					i += m.Length;
					continue;
				}
				if (c == '"' || c == '\'')
				{
					int st = i++;
					while (i < s.Length && s[i] != c && s[i] != '\n') { if (s[i] == '\\') i++; i++; }
					i = Math.Min(i + 1, s.Length);
					list.Add(new Tok(TK.String, s[st..i], line));
					continue;
				}
				string? mp = null;
				foreach (var p in MultiPunct)
					if (string.CompareOrdinal(s, i, p, 0, p.Length) == 0) { mp = p; break; }
				if (mp != null) { list.Add(new Tok(TK.Punct, mp, line)); i += mp.Length; continue; }
				list.Add(new Tok(TK.Punct, c.ToString(), line));
				i++;
			}
			if (!keepSpace) list.Add(new Tok(TK.EOF, "", line));
			return list;
		}
	}

	// ============================================================== preprocessor

	static class Preprocessor
	{
		sealed class Macro
		{
			public List<string>? Params;
			public List<Tok> Body = [];
		}

		static string StripComments(string s)
		{
			var sb = new StringBuilder(s.Length);
			for (int i = 0; i < s.Length; i++)
			{
				char c = s[i];
				if (c == '/' && i + 1 < s.Length && s[i + 1] == '/')
				{
					while (i < s.Length && s[i] != '\n') i++;
					if (i < s.Length) sb.Append('\n');
					continue;
				}
				if (c == '/' && i + 1 < s.Length && s[i + 1] == '*')
				{
					int end = s.IndexOf("*/", i + 2, StringComparison.Ordinal);
					if (end < 0) end = s.Length - 2;
					for (int k = i; k < end + 2 && k < s.Length; k++) if (s[k] == '\n') sb.Append('\n');
					sb.Append(' ');
					i = end + 1;
					continue;
				}
				if (c == '"')
				{
					int st = i++;
					while (i < s.Length && s[i] != '"' && s[i] != '\n') { if (s[i] == '\\') i++; i++; }
					sb.Append(s, st, Math.Min(i + 1, s.Length) - st);
					continue;
				}
				sb.Append(c);
			}
			return sb.ToString();
		}

		public static string Run(string src, List<string> warnings)
		{
			src = src.Replace("\r\n", "\n").Replace('\r', '\n').Replace('\0', ' ');
			src = StripComments(src);
			src = Regex.Replace(src, @"\\\n", " ");
			var macros = new Dictionary<string, Macro>();
			var outSb = new StringBuilder();
			var cond = new Stack<(bool active, bool taken, bool parent)>();
			bool Active() => cond.Count == 0 || cond.Peek().active;

			foreach (var rawLine in src.Split('\n'))
			{
				string line = rawLine.TrimStart();
				if (line.StartsWith('#'))
				{
					var m = Regex.Match(line, @"^#\s*(\w*)\s*(.*)$");
					string dir = m.Groups[1].Value, rest = m.Groups[2].Value.Trim();
					switch (dir)
					{
						case "ifdef":
						case "ifndef":
						{
							bool def = macros.ContainsKey(rest.Split(' ', '\t')[0]);
							bool v = dir == "ifdef" ? def : !def;
							bool p = Active();
							cond.Push((p && v, v, p));
							break;
						}
						case "if":
						{
							bool p = Active();
							bool v = p && Eval(rest, macros);
							cond.Push((p && v, v, p));
							break;
						}
						case "elif":
						{
							if (cond.Count == 0) break;
							var top = cond.Pop();
							bool v = !top.taken && top.parent && Eval(rest, macros);
							cond.Push((top.parent && v, top.taken || v, top.parent));
							break;
						}
						case "else":
						{
							if (cond.Count == 0) break;
							var top = cond.Pop();
							cond.Push((top.parent && !top.taken, true, top.parent));
							break;
						}
						case "endif":
							if (cond.Count > 0) cond.Pop();
							break;
						case "define" when Active():
						{
							var dm = Regex.Match(rest, @"^(\w+)(\(([^)]*)\))?(.*)$", RegexOptions.Singleline);
							if (!dm.Success) break;
							var mac = new Macro();
							if (dm.Groups[2].Success)
								mac.Params = dm.Groups[3].Value.Split(',').Select(x => x.Trim()).Where(x => x.Length > 0).ToList();
							mac.Body = Lexer.Lex(dm.Groups[4].Value.Trim(), keepSpace: true);
							macros[dm.Groups[1].Value] = mac;
							break;
						}
						case "undef" when Active():
							macros.Remove(rest.Split(' ', '\t')[0]);
							break;
						case "include" when Active():
							warnings.Add($"#include {rest} is not supported.");
							break;
					}
					outSb.Append('\n');
					continue;
				}
				if (Active())
				{
					var toks = Lexer.Lex(rawLine, keepSpace: true);
					var exp = Expand(toks, macros, []);
					foreach (var t in exp) outSb.Append(t.Text);
				}
				outSb.Append('\n');
			}
			return outSb.ToString();
		}

		static List<Tok> Expand(List<Tok> toks, Dictionary<string, Macro> macros, HashSet<string> active)
		{
			var result = new List<Tok>();
			for (int i = 0; i < toks.Count; i++)
			{
				var t = toks[i];
				if (t.Kind != TK.Ident || !macros.TryGetValue(t.Text, out var mac) || active.Contains(t.Text))
				{
					result.Add(t);
					continue;
				}
				var nested = new HashSet<string>(active) { t.Text };
				if (mac.Params == null)
				{
					result.Add(new Tok(TK.Space, " "));
					result.AddRange(Expand(Paste(mac.Body), macros, nested));
					result.Add(new Tok(TK.Space, " "));
					continue;
				}
				int j = i + 1;
				while (j < toks.Count && toks[j].Kind == TK.Space) j++;
				if (j >= toks.Count || toks[j].Text != "(") { result.Add(t); continue; }
				var args = new List<List<Tok>>();
				var cur = new List<Tok>();
				int depth = 0, k = j;
				for (; k < toks.Count; k++)
				{
					var x = toks[k];
					if (x.Kind == TK.Punct && x.Text == "(") { if (depth++ == 0) continue; }
					else if (x.Kind == TK.Punct && x.Text == ")") { if (--depth == 0) break; }
					else if (x.Kind == TK.Punct && x.Text == "," && depth == 1) { args.Add(cur); cur = []; continue; }
					cur.Add(x);
				}
				args.Add(cur);
				if (mac.Params.Count == 0 && args.Count == 1 && args[0].All(a => a.Kind == TK.Space)) args.Clear();
				var expandedArgs = args.Select(a => Expand(a, macros, active)).ToList();
				var body = new List<Tok>();
				for (int b = 0; b < mac.Body.Count; b++)
				{
					var bt = mac.Body[b];
					if (bt.Kind == TK.Punct && bt.Text == "#" && b + 1 < mac.Body.Count && mac.Body[b + 1].Kind == TK.Ident)
					{
						int pi = mac.Params.IndexOf(mac.Body[b + 1].Text);
						if (pi >= 0)
						{
							body.Add(new Tok(TK.String, "\"" + string.Concat(pi < args.Count ? args[pi].Select(a => a.Text) : []).Trim() + "\""));
							b++;
							continue;
						}
					}
					int idx = bt.Kind == TK.Ident ? mac.Params.IndexOf(bt.Text) : -1;
					if (idx >= 0) body.AddRange(idx < expandedArgs.Count ? expandedArgs[idx] : []);
					else body.Add(bt);
				}
				result.Add(new Tok(TK.Space, " "));
				result.AddRange(Expand(Paste(body), macros, nested));
				result.Add(new Tok(TK.Space, " "));
				i = k;
			}
			return result;
		}

		static List<Tok> Paste(List<Tok> toks)
		{
			if (!toks.Any(t => t.Text == "##")) return toks;
			var r = new List<Tok>();
			for (int i = 0; i < toks.Count; i++)
			{
				if (toks[i].Text == "##")
				{
					while (r.Count > 0 && r[^1].Kind == TK.Space) r.RemoveAt(r.Count - 1);
					int j = i + 1;
					while (j < toks.Count && toks[j].Kind == TK.Space) j++;
					if (r.Count > 0 && j < toks.Count)
					{
						var merged = r[^1].Text + toks[j].Text;
						r[^1] = Lexer.Lex(merged, true).FirstOrDefault(new Tok(TK.Ident, merged));
						i = j;
					}
					continue;
				}
				r.Add(toks[i]);
			}
			return r;
		}

		static bool Eval(string expr, Dictionary<string, Macro> macros)
		{
			expr = Regex.Replace(expr, @"defined\s*\(\s*(\w+)\s*\)|defined\s+(\w+)", m =>
				macros.ContainsKey(m.Groups[1].Success ? m.Groups[1].Value : m.Groups[2].Value) ? "1" : "0");
			var toks = Expand(Lexer.Lex(expr, true), macros, []).Where(t => t.Kind != TK.Space).ToList();
			int pos = 0;
			try { return EvalTernary(toks, ref pos) != 0; }
			catch { return false; }
		}

		static long EvalTernary(List<Tok> t, ref int p)
		{
			long c = EvalBin(t, ref p, 0);
			if (p < t.Count && t[p].Text == "?")
			{
				p++;
				long a = EvalTernary(t, ref p);
				if (p < t.Count && t[p].Text == ":") p++;
				long b = EvalTernary(t, ref p);
				return c != 0 ? a : b;
			}
			return c;
		}

		static readonly string[][] Levels = [["||"], ["&&"], ["|"], ["^"], ["&"], ["==", "!="], ["<", ">", "<=", ">="], ["<<", ">>"], ["+", "-"], ["*", "/", "%"]];

		static long EvalBin(List<Tok> t, ref int p, int level)
		{
			if (level >= Levels.Length) return EvalUnary(t, ref p);
			long l = EvalBin(t, ref p, level + 1);
			while (p < t.Count && Levels[level].Contains(t[p].Text))
			{
				string op = t[p++].Text;
				long r = EvalBin(t, ref p, level + 1);
				l = op switch
				{
					"||" => (l != 0 || r != 0) ? 1 : 0,
					"&&" => (l != 0 && r != 0) ? 1 : 0,
					"|" => l | r, "^" => l ^ r, "&" => l & r,
					"==" => l == r ? 1 : 0, "!=" => l != r ? 1 : 0,
					"<" => l < r ? 1 : 0, ">" => l > r ? 1 : 0, "<=" => l <= r ? 1 : 0, ">=" => l >= r ? 1 : 0,
					"<<" => l << (int)r, ">>" => l >> (int)r,
					"+" => l + r, "-" => l - r, "*" => l * r,
					"/" => r == 0 ? 0 : l / r, "%" => r == 0 ? 0 : l % r,
					_ => 0
				};
			}
			return l;
		}

		static long EvalUnary(List<Tok> t, ref int p)
		{
			if (p >= t.Count) return 0;
			var x = t[p];
			if (x.Text == "!") { p++; return EvalUnary(t, ref p) == 0 ? 1 : 0; }
			if (x.Text == "-") { p++; return -EvalUnary(t, ref p); }
			if (x.Text == "+") { p++; return EvalUnary(t, ref p); }
			if (x.Text == "~") { p++; return ~EvalUnary(t, ref p); }
			if (x.Text == "(") { p++; long v = EvalTernary(t, ref p); if (p < t.Count && t[p].Text == ")") p++; return v; }
			p++;
			if (x.Kind == TK.Number)
			{
				string n = x.Text.TrimEnd('u', 'U', 'l', 'L', 'f', 'F', 'h', 'H');
				if (n.StartsWith("0x", StringComparison.OrdinalIgnoreCase)) return System.Convert.ToInt64(n, 16);
				return double.TryParse(n, NumberStyles.Float, CultureInfo.InvariantCulture, out var d) ? (long)d : 0;
			}
			return 0; // undefined identifier
		}
	}

	// ============================================================== types

	enum Kind { Float, Bool, Int, Void, Sampler, Struct, Unknown }

	sealed class Ty
	{
		public Kind Kind;
		public int Rows = 1, Cols = 1; // vectors: Rows = 1, Cols = n; matrices: HLSL rows x cols
		public bool IsMatrix;
		public string? Struct;
		public List<int> Dims = []; // array dimensions (-1 = unsized)

		public static readonly Ty Float1 = new() { Kind = Kind.Float };
		public static readonly Ty Bool1 = new() { Kind = Kind.Bool };
		public static readonly Ty Int1 = new() { Kind = Kind.Int };
		public static readonly Ty Void = new() { Kind = Kind.Void };
		public static readonly Ty Sampler = new() { Kind = Kind.Sampler };
		public static readonly Ty Unknown = new() { Kind = Kind.Unknown };

		public static Ty Vec(int n, Kind k = Kind.Float) => new() { Kind = k, Cols = n };
		public static Ty Mat(int r, int c) => new() { Kind = Kind.Float, Rows = r, Cols = c, IsMatrix = true };

		public bool IsArray => Dims.Count > 0;
		public bool IsScalar => !IsArray && !IsMatrix && Cols == 1 && Kind is Kind.Float or Kind.Bool or Kind.Int;
		public bool IsVector => !IsArray && !IsMatrix && Cols > 1 && Kind is Kind.Float or Kind.Bool or Kind.Int;
		public bool IsNumeric => !IsArray && Kind is Kind.Float or Kind.Bool or Kind.Int;
		public int Comps => IsMatrix ? Rows * Cols : Cols;

		public Ty Element() => new() { Kind = Kind, Rows = Rows, Cols = Cols, IsMatrix = IsMatrix, Struct = Struct };
		public Ty WithKind(Kind k) => new() { Kind = k, Rows = Rows, Cols = Cols, IsMatrix = IsMatrix, Struct = Struct, Dims = Dims };
		public Ty WithDims(List<int> d) => new() { Kind = Kind, Rows = Rows, Cols = Cols, IsMatrix = IsMatrix, Struct = Struct, Dims = d };

		public bool SameAs(Ty o) => Kind == o.Kind && Rows == o.Rows && Cols == o.Cols && IsMatrix == o.IsMatrix && Struct == o.Struct && Dims.SequenceEqual(o.Dims);

		/// <summary>GLSL type name, without array dimensions.</summary>
		public string Glsl()
		{
			switch (Kind)
			{
				case Kind.Void: return "void";
				case Kind.Sampler: return "sampler2D";
				case Kind.Struct: return Struct!;
				case Kind.Unknown: return "float";
				case Kind.Int: return Cols == 1 ? "int" : $"ivec{Cols}";
			}
			if (IsMatrix)
			{
				if (Rows == 1 || Cols == 1) return Rows * Cols == 1 ? "float" : $"vec{Rows * Cols}";
				return Rows == Cols ? $"mat{Rows}" : $"mat{Rows}x{Cols}";
			}
			if (Kind == Kind.Bool) return Cols == 1 ? "bool" : $"bvec{Cols}";
			return Cols == 1 ? "float" : $"vec{Cols}";
		}

		// multi-dimensional arrays are flattened (GLSL 330 / ES 300 have no arrays of arrays)
		public string ArraySuffix() => Dims.Count == 0 ? "" : Dims.Count > 1 ? $"[{Dims.Aggregate(1, (a, b) => a * Math.Max(b, 1))}]" : Dims[0] < 0 ? "[]" : $"[{Dims[0]}]";

		public override string ToString() => Glsl() + ArraySuffix();
	}

	// ============================================================== AST

	abstract class Expr { public int Line; }
	sealed class LitExpr : Expr { public string Text = ""; public bool IsBool; }
	sealed class IdentExpr : Expr { public string Name = ""; }
	sealed class UnaryExpr : Expr { public string Op = ""; public Expr E = null!; public bool Postfix; }
	sealed class BinaryExpr : Expr { public string Op = ""; public Expr L = null!, R = null!; }
	sealed class TernaryExpr : Expr { public Expr C = null!, T = null!, F = null!; }
	sealed class CallExpr : Expr { public string Name = ""; public TypeRef? Ctor; public List<Expr> Args = []; public Expr? Object; }
	sealed class CastExpr : Expr { public TypeRef T = null!; public Expr E = null!; }
	sealed class MemberExpr : Expr { public Expr E = null!; public string Name = ""; }
	sealed class IndexExpr : Expr { public Expr E = null!, I = null!; }
	sealed class InitListExpr : Expr { public List<Expr> Items = []; }
	sealed class ParenExpr : Expr { public Expr E = null!; }

	sealed class TypeRef { public string Name = ""; public Ty? Resolved; }

	sealed class VarDecl
	{
		public TypeRef Type = null!;
		public string Name = "";
		public List<Expr?> Dims = [];
		public string Semantic = "";
		public string Register = "";
		public Expr? Init;
		public bool IsStatic, IsConst, IsUniform, IsSamplerState;
		public string ParamMod = "in";
		public int Line;
	}

	abstract class Stmt { public int Line; }
	sealed class BlockStmt : Stmt { public List<Stmt> Body = []; }
	sealed class DeclStmt : Stmt { public List<VarDecl> Decls = []; }
	sealed class ExprStmt : Stmt { public Expr? E; }
	sealed class IfStmt : Stmt { public Expr C = null!; public Stmt T = null!; public Stmt? F; }
	sealed class ForStmt : Stmt { public Stmt? Init; public Expr? C; public Expr? Step; public Stmt Body = null!; }
	sealed class WhileStmt : Stmt { public Expr C = null!; public Stmt Body = null!; public bool Do; }
	sealed class ReturnStmt : Stmt { public Expr? E; }
	sealed class JumpStmt : Stmt { public string Kind = ""; }

	sealed class StructDef { public string Name = ""; public List<VarDecl> Members = []; }

	sealed class FuncDef
	{
		public TypeRef Ret = null!;
		public string Name = "";
		public string RetSemantic = "";
		public List<VarDecl> Params = [];
		public BlockStmt? Body;
		public int Line;
	}

	sealed class Unit
	{
		public List<object> Items = []; // StructDef, FuncDef, List<VarDecl>
		public string? Entry;
	}

	// ============================================================== parser

	static readonly HashSet<string> TypeModifiers = ["static", "const", "uniform", "extern", "shared", "volatile", "row_major", "column_major", "groupshared", "inline", "precise", "nointerpolation", "noperspective", "centroid", "linear", "sample", "in", "out", "inout", "snorm", "unorm"];

	static readonly HashSet<string> Attributes = ["unroll", "loop", "branch", "flatten", "fastopt", "call", "forcecase", "allow_uav_condition", "isolate", "maxexports"];

	sealed class Parser(List<Tok> toks)
	{
		int p;
		readonly HashSet<string> structNames = [];

		Tok Peek(int o = 0) => toks[Math.Min(p + o, toks.Count - 1)];
		bool At(string s, int o = 0) { var t = Peek(o); return t.Kind is TK.Punct or TK.Ident && t.Text == s; }
		Tok Next() => toks[Math.Min(p++, toks.Count - 1)];
		bool Accept(string s) { if (At(s)) { p++; return true; } return false; }
		void Expect(string s)
		{
			if (!Accept(s)) throw new TranslateException($"line {Peek().Line}: expected '{s}' but found '{Peek().Text}'");
		}
		string ExpectIdent()
		{
			var t = Next();
			if (t.Kind != TK.Ident) throw new TranslateException($"line {t.Line}: expected identifier but found '{t.Text}'");
			return t.Text;
		}

		public bool IsTypeName(string s) => structNames.Contains(s) || BaseType(s) != null || s is "vector" or "matrix";

		public static Ty? BaseType(string s)
		{
			switch (s)
			{
				case "void": return Ty.Void;
				case "bool": return Ty.Bool1;
				case "float" or "half" or "double" or "fixed" or "min16float" or "min10float" or "int" or "uint" or "dword" or "min16int" or "min12int" or "min16uint" or "real":
					return Ty.Float1;
				case "sampler" or "sampler2D" or "sampler1D" or "samplerRECT" or "sampler3D" or "samplerCUBE" or "SamplerState" or "sampler_state":
					return Ty.Sampler;
				case "texture" or "Texture" or "texture2D" or "Texture2D" or "texture1D" or "texture3D" or "textureCUBE" or "Texture1D" or "Texture3D" or "TextureCube":
					return new Ty { Kind = Kind.Sampler, Struct = "texture" };
			}
			var m = Regex.Match(s, @"^(float|half|double|fixed|min16float|min10float|int|uint|dword|min16int|min16uint|bool)([1-4])(?:x([1-4]))?$");
			if (!m.Success) return null;
			var k = m.Groups[1].Value == "bool" ? Kind.Bool : Kind.Float;
			int a = int.Parse(m.Groups[2].Value);
			if (m.Groups[3].Success)
			{
				int b = int.Parse(m.Groups[3].Value);
				if (a == 1 && b == 1) return new Ty { Kind = k };
				if (a == 1 || b == 1) return Ty.Vec(Math.Max(a, b), k);
				return Ty.Mat(a, b);
			}
			return a == 1 ? new Ty { Kind = k } : Ty.Vec(a, k);
		}

		TypeRef ParseType()
		{
			string name = ExpectIdent();
			if ((name == "vector" || name == "matrix") && At("<"))
			{
				Next();
				string bt = ExpectIdent();
				var nums = new List<string>();
				while (Accept(",")) nums.Add(Next().Text);
				Expect(">");
				name = name == "vector" ? $"{bt}{(nums.Count > 0 ? nums[0] : "4")}" : $"{bt}{(nums.Count > 0 ? nums[0] : "4")}x{(nums.Count > 1 ? nums[1] : "4")}";
			}
			return new TypeRef { Name = name };
		}

		void SkipBalanced(string open, string close)
		{
			int depth = 0;
			do
			{
				var t = Next();
				if (t.Kind == TK.EOF) return;
				if (t.Text == open) depth++;
				else if (t.Text == close || (close == ">" && t.Text == ">=")) depth--;
			} while (depth > 0);
		}

		void SkipAttributes()
		{
			while (At("[") && Peek(1).Kind == TK.Ident && Attributes.Contains(Peek(1).Text))
				SkipBalanced("[", "]");
		}

		public Unit ParseUnit()
		{
			var unit = new Unit();
			while (Peek().Kind != TK.EOF)
			{
				if (Accept(";")) continue;
				var t = Peek();
				if (t.Text is "technique" or "technique9" or "technique10" or "technique11")
				{
					ParseTechnique(unit);
					continue;
				}
				if (t.Text == "struct" && Peek(2).Text == "{")
				{
					unit.Items.Add(ParseStruct());
					continue;
				}
				if (t.Text == "typedef") throw new TranslateException("typedef is not supported");
				if (t.Text == "cbuffer" || t.Text == "tbuffer")
				{
					Next(); ExpectIdent();
					if (Accept(":")) { while (!At("{") && Peek().Kind != TK.EOF) Next(); }
					Expect("{");
					while (!At("}") && Peek().Kind != TK.EOF) ParseGlobalOrFunction(unit);
					Expect("}");
					continue;
				}
				ParseGlobalOrFunction(unit);
			}
			return unit;
		}

		void ParseTechnique(Unit unit)
		{
			Next();
			if (Peek().Kind == TK.Ident) Next();
			if (At("<")) SkipBalanced("<", ">");
			int start = p;
			SkipBalanced("{", "}");
			for (int i = start; i < p - 2; i++)
			{
				if (toks[i].Text.Equals("PixelShader", StringComparison.OrdinalIgnoreCase))
				{
					for (int j = i + 1; j < p - 1 && toks[j].Text != ";"; j++)
						if (toks[j].Text == "compile" && j + 2 < p) { unit.Entry ??= toks[j + 2].Text; break; }
				}
			}
		}

		StructDef ParseStruct()
		{
			Expect("struct");
			var sd = new StructDef { Name = ExpectIdent() };
			structNames.Add(sd.Name);
			Expect("{");
			while (!Accept("}"))
			{
				if (Peek().Kind == TK.EOF) throw new TranslateException("unterminated struct");
				while (Peek().Kind == TK.Ident && TypeModifiers.Contains(Peek().Text) && Peek(1).Kind == TK.Ident && !(Peek(2).Text is ";" or ":" or "," or "[")) Next();
				var type = ParseType();
				do sd.Members.Add(ParseDeclarator(type, isGlobal: false));
				while (Accept(","));
				Expect(";");
			}
			return sd;
		}

		void ParseGlobalOrFunction(Unit unit)
		{
			int line = Peek().Line;
			bool isStatic = false, isConst = false, isUniform = false;
			while (Peek().Kind == TK.Ident && TypeModifiers.Contains(Peek().Text) && Peek(1).Kind == TK.Ident)
			{
				var m = Next().Text;
				if (m == "static") isStatic = true;
				if (m == "const") isConst = true;
				if (m == "uniform") isUniform = true;
			}
			var type = ParseType();
			if (!IsTypeName(type.Name)) throw new TranslateException($"line {line}: unknown type '{type.Name}'");
			if (Peek().Kind == TK.Ident && At("(", 1))
			{
				unit.Items.Add(ParseFunction(type, line));
				return;
			}
			var decls = new List<VarDecl>();
			do
			{
				var d = ParseDeclarator(type, isGlobal: true);
				d.IsStatic = isStatic; d.IsConst = isConst; d.IsUniform = isUniform; d.Line = line;
				decls.Add(d);
			} while (Accept(","));
			Expect(";");
			unit.Items.Add(decls);
		}

		VarDecl ParseDeclarator(TypeRef type, bool isGlobal)
		{
			var d = new VarDecl { Type = type, Name = ExpectIdent(), Line = Peek().Line };
			while (Accept("["))
			{
				d.Dims.Add(At("]") ? null : ParseExpr());
				Expect("]");
			}
			while (Accept(":"))
			{
				if (At("register") || At("packoffset"))
				{
					bool reg = Next().Text == "register";
					Expect("(");
					var sb = new StringBuilder();
					while (!At(")") && Peek().Kind != TK.EOF) sb.Append(Next().Text);
					Expect(")");
					if (reg) d.Register = sb.ToString().ToLowerInvariant();
				}
				else d.Semantic = ExpectIdent().ToUpperInvariant();
			}
			if (At("<")) SkipBalanced("<", ">"); // annotations
			if (Accept("="))
			{
				if (At("sampler_state") || At("compile"))
				{
					Next();
					if (At("{")) SkipBalanced("{", "}");
					d.IsSamplerState = true;
				}
				else if (At("{")) d.Init = ParseInitList();
				else d.Init = ParseAssign();
			}
			else if (At("{") && isGlobal)
			{
				SkipBalanced("{", "}");
				d.IsSamplerState = true;
			}
			return d;
		}

		InitListExpr ParseInitList()
		{
			var il = new InitListExpr { Line = Peek().Line };
			Expect("{");
			while (!At("}"))
			{
				il.Items.Add(At("{") ? ParseInitList() : ParseAssign());
				if (!Accept(",")) break;
			}
			Expect("}");
			return il;
		}

		FuncDef ParseFunction(TypeRef ret, int line)
		{
			var f = new FuncDef { Ret = ret, Name = ExpectIdent(), Line = line };
			Expect("(");
			if (At("void") && At(")", 1)) Next();
			while (!At(")"))
			{
				var mods = new List<string>();
				while (Peek().Kind == TK.Ident && TypeModifiers.Contains(Peek().Text) && Peek(1).Kind == TK.Ident && Peek(2).Kind == TK.Ident)
					mods.Add(Next().Text);
				var pt = ParseType();
				var d = new VarDecl { Type = pt, Name = ExpectIdent(), Line = Peek().Line };
				d.ParamMod = mods.Contains("inout") ? "inout" : mods.Contains("out") ? "out" : "in";
				d.IsUniform = mods.Contains("uniform");
				d.IsConst = mods.Contains("const");
				while (Accept("[")) { d.Dims.Add(At("]") ? null : ParseExpr()); Expect("]"); }
				while (Accept(":"))
				{
					if (At("register")) { Next(); SkipBalanced("(", ")"); }
					else d.Semantic = ExpectIdent().ToUpperInvariant();
				}
				if (At("<")) SkipBalanced("<", ">");
				if (Accept("=")) d.Init = ParseAssign();
				f.Params.Add(d);
				if (!Accept(",")) break;
			}
			Expect(")");
			if (Accept(":")) f.RetSemantic = ExpectIdent().ToUpperInvariant();
			if (Accept(";")) return f;
			f.Body = ParseBlock();
			return f;
		}

		BlockStmt ParseBlock()
		{
			var b = new BlockStmt { Line = Peek().Line };
			Expect("{");
			while (!Accept("}"))
			{
				if (Peek().Kind == TK.EOF) throw new TranslateException("unterminated block");
				b.Body.Add(ParseStmt());
			}
			return b;
		}

		bool LooksLikeDecl()
		{
			int o = 0;
			while (Peek(o).Kind == TK.Ident && TypeModifiers.Contains(Peek(o).Text) && Peek(o + 1).Kind == TK.Ident) o++;
			var t = Peek(o);
			if (t.Kind != TK.Ident || !IsTypeName(t.Text)) return false;
			if (t.Text is "vector" or "matrix" && Peek(o + 1).Text == "<") return true;
			return Peek(o + 1).Kind == TK.Ident;
		}

		DeclStmt ParseLocalDecl()
		{
			var ds = new DeclStmt { Line = Peek().Line };
			bool isConst = false, isStatic = false;
			while (Peek().Kind == TK.Ident && TypeModifiers.Contains(Peek().Text) && Peek(1).Kind == TK.Ident)
			{
				var m = Next().Text;
				if (m == "const") isConst = true;
				if (m == "static") isStatic = true;
			}
			var type = ParseType();
			do
			{
				var d = ParseDeclarator(type, isGlobal: false);
				d.IsConst = isConst; d.IsStatic = isStatic;
				ds.Decls.Add(d);
			} while (Accept(","));
			return ds;
		}

		Stmt ParseStmt()
		{
			SkipAttributes();
			int line = Peek().Line;
			if (At("{")) return ParseBlock();
			if (Accept(";")) return new ExprStmt { Line = line };
			if (Accept("if"))
			{
				Expect("(");
				var c = ParseExpr();
				Expect(")");
				var s = new IfStmt { C = c, T = ParseStmt(), Line = line };
				if (Accept("else")) s.F = ParseStmt();
				return s;
			}
			if (Accept("for"))
			{
				Expect("(");
				var f = new ForStmt { Line = line };
				if (!At(";")) f.Init = LooksLikeDecl() ? ParseLocalDecl() : new ExprStmt { E = ParseExpr(), Line = line };
				Expect(";");
				if (!At(";")) f.C = ParseExpr();
				Expect(";");
				if (!At(")")) f.Step = ParseExpr();
				Expect(")");
				f.Body = ParseStmt();
				return f;
			}
			if (Accept("while"))
			{
				Expect("(");
				var c = ParseExpr();
				Expect(")");
				return new WhileStmt { C = c, Body = ParseStmt(), Line = line };
			}
			if (Accept("do"))
			{
				var body = ParseStmt();
				Expect("while");
				Expect("(");
				var c = ParseExpr();
				Expect(")");
				Expect(";");
				return new WhileStmt { C = c, Body = body, Do = true, Line = line };
			}
			if (Accept("return"))
			{
				var r = new ReturnStmt { Line = line };
				if (!At(";")) r.E = ParseExpr();
				Expect(";");
				return r;
			}
			if (At("break") || At("continue") || At("discard"))
			{
				var j = new JumpStmt { Kind = Next().Text, Line = line };
				Expect(";");
				return j;
			}
			if (At("switch")) throw new TranslateException("switch statements are not supported");
			if (LooksLikeDecl())
			{
				var d = ParseLocalDecl();
				Expect(";");
				return d;
			}
			var e = new ExprStmt { E = ParseExpr(), Line = line };
			Expect(";");
			return e;
		}

		// ---- expressions

		Expr ParseExpr()
		{
			var e = ParseAssign();
			while (At(","))
			{
				int line = Next().Line;
				e = new BinaryExpr { Op = ",", L = e, R = ParseAssign(), Line = line };
			}
			return e;
		}

		static readonly HashSet<string> AssignOps = ["=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>="];

		Expr ParseAssign()
		{
			var l = ParseTernary();
			if (Peek().Kind == TK.Punct && AssignOps.Contains(Peek().Text))
			{
				var op = Next();
				var r = ParseAssign();
				return new BinaryExpr { Op = op.Text, L = l, R = r, Line = op.Line };
			}
			return l;
		}

		Expr ParseTernary()
		{
			var c = ParseBinary(0);
			if (At("?"))
			{
				int line = Next().Line;
				var t = ParseAssign();
				Expect(":");
				var f = ParseAssign();
				return new TernaryExpr { C = c, T = t, F = f, Line = line };
			}
			return c;
		}

		static readonly string[][] Levels = [["||"], ["&&"], ["|"], ["^"], ["&"], ["==", "!="], ["<", ">", "<=", ">="], ["<<", ">>"], ["+", "-"], ["*", "/", "%"]];

		Expr ParseBinary(int level)
		{
			if (level >= Levels.Length) return ParseUnary();
			var l = ParseBinary(level + 1);
			while (Peek().Kind == TK.Punct && Levels[level].Contains(Peek().Text))
			{
				var op = Next();
				var r = ParseBinary(level + 1);
				l = new BinaryExpr { Op = op.Text, L = l, R = r, Line = op.Line };
			}
			return l;
		}

		Expr ParseUnary()
		{
			var t = Peek();
			if (t.Kind == TK.Punct && t.Text is "-" or "+" or "!" or "~" or "++" or "--")
			{
				Next();
				return new UnaryExpr { Op = t.Text, E = ParseUnary(), Line = t.Line };
			}
			if (At("(") && Peek(1).Kind == TK.Ident && IsTypeName(Peek(1).Text) && (At(")", 2) || (Peek(1).Text is "vector" or "matrix" && At("<", 2))))
			{
				Next();
				var type = ParseType();
				Expect(")");
				return new CastExpr { T = type, E = ParseUnary(), Line = t.Line };
			}
			return ParsePostfix(ParsePrimary());
		}

		Expr ParsePrimary()
		{
			var t = Next();
			switch (t.Kind)
			{
				case TK.Number:
					return new LitExpr { Text = t.Text, Line = t.Line };
				case TK.Ident:
					if (t.Text is "true" or "false") return new LitExpr { Text = t.Text, IsBool = true, Line = t.Line };
					if (IsTypeName(t.Text) && (At("(") || (t.Text is "vector" or "matrix" && At("<"))))
					{
						p--;
						var type = ParseType();
						Expect("(");
						var call = new CallExpr { Name = type.Name, Ctor = type, Line = t.Line };
						ParseArgs(call.Args);
						return call;
					}
					if (At("("))
					{
						Next();
						var call = new CallExpr { Name = t.Text, Line = t.Line };
						ParseArgs(call.Args);
						return call;
					}
					return new IdentExpr { Name = t.Text, Line = t.Line };
				case TK.Punct when t.Text == "(":
					var e = ParseExpr();
					Expect(")");
					return new ParenExpr { E = e, Line = t.Line };
				case TK.Punct when t.Text == "{":
					p--;
					return ParseInitList();
			}
			throw new TranslateException($"line {t.Line}: unexpected '{t.Text}'");
		}

		void ParseArgs(List<Expr> args)
		{
			while (!At(")"))
			{
				args.Add(ParseAssign());
				if (!Accept(",")) break;
			}
			Expect(")");
		}

		Expr ParsePostfix(Expr e)
		{
			while (true)
			{
				var t = Peek();
				if (At("."))
				{
					Next();
					string name = ExpectIdent();
					if (At("("))
					{
						Next();
						var call = new CallExpr { Name = name, Object = e, Line = t.Line };
						ParseArgs(call.Args);
						e = call;
					}
					else e = new MemberExpr { E = e, Name = name, Line = t.Line };
				}
				else if (At("["))
				{
					Next();
					var i = ParseExpr();
					Expect("]");
					e = new IndexExpr { E = e, I = i, Line = t.Line };
				}
				else if (At("++") || At("--"))
				{
					Next();
					e = new UnaryExpr { Op = t.Text, E = e, Postfix = true, Line = t.Line };
				}
				else return e;
			}
		}
	}

	// ============================================================== emitter

	// GLSL words that are legal HLSL identifiers (keywords, reserved words, builtin functions, our own names).
	static readonly HashSet<string> GlslReserved =
	[
		"attribute", "varying", "layout", "centroid", "flat", "smooth", "noperspective", "patch", "sample", "subroutine",
		"invariant", "precise", "lowp", "mediump", "highp", "precision", "common", "partition", "active", "asm", "class",
		"union", "enum", "typedef", "template", "this", "resource", "goto", "inline", "noinline", "public", "external",
		"interface", "long", "short", "half", "fixed", "unsigned", "superp", "input", "output", "filter", "sizeof", "cast",
		"namespace", "using", "buffer", "shared", "coherent", "volatile", "restrict", "readonly", "writeonly", "atomic_uint",
		"vec2", "vec3", "vec4", "ivec2", "ivec3", "ivec4", "bvec2", "bvec3", "bvec4", "uvec2", "uvec3", "uvec4",
		"dvec2", "dvec3", "dvec4", "hvec2", "hvec3", "hvec4", "fvec2", "fvec3", "fvec4",
		"mat2", "mat3", "mat4", "mat2x2", "mat2x3", "mat2x4", "mat3x2", "mat3x3", "mat3x4", "mat4x2", "mat4x3", "mat4x4",
		"sampler2D", "sampler3D", "samplerCube", "sampler2DShadow", "isampler2D", "usampler2D", "image2D", "texture",
		"radians", "degrees", "sin", "cos", "tan", "asin", "acos", "atan", "sinh", "cosh", "tanh", "asinh", "acosh", "atanh",
		"pow", "exp", "log", "exp2", "log2", "sqrt", "inversesqrt", "abs", "sign", "floor", "trunc", "round", "roundEven",
		"ceil", "fract", "mod", "modf", "min", "max", "clamp", "mix", "step", "smoothstep", "isnan", "isinf",
		"floatBitsToInt", "floatBitsToUint", "intBitsToFloat", "uintBitsToFloat", "packSnorm2x16", "unpackSnorm2x16",
		"packUnorm2x16", "unpackUnorm2x16", "packHalf2x16", "unpackHalf2x16", "length", "distance", "dot", "cross",
		"normalize", "faceforward", "reflect", "refract", "matrixCompMult", "outerProduct", "transpose", "determinant",
		"inverse", "lessThan", "lessThanEqual", "greaterThan", "greaterThanEqual", "equal", "notEqual", "any", "all", "not",
		"textureSize", "textureProj", "textureLod", "textureOffset", "texelFetch", "texelFetchOffset", "textureProjOffset",
		"textureLodOffset", "textureProjLod", "textureProjLodOffset", "textureGrad", "textureGradOffset", "textureProjGrad",
		"textureProjGradOffset", "dFdx", "dFdy", "fwidth",
		"main", "TexCoord", "FragColor", "uColor", "uTexture", "uMVP", "uInkEffect", "fmod", "sincos"
	];

	static readonly HashSet<string> RuntimeUniforms = ["fPixelWidth", "fPixelHeight"];

	sealed class Symbol
	{
		public string Glsl = "";
		public Ty Type = Ty.Unknown;
		public Ty? Storage;          // GLSL storage type when it differs (uniforms declared with the runtime's type)
		public bool IsConst, IsUniform;
		public string? WritableCopy; // uniforms assigned to in code read/write this global instead
	}

	sealed class Value(string code, Ty type, int prec = 100, bool isConst = false)
	{
		public string Code = code;
		public Ty Type = type;
		public int Prec = prec; // 100 = primary
		public bool IsConst = isConst;
		// multi-dimensional array partial index
		public string? ArrayBase;
		public string? LinearIndex;
		public List<int>? RemainingDims;
	}

	sealed class FuncSig
	{
		public string Glsl = "";
		public Ty Ret = Ty.Void;
		public List<(Ty Type, string Mod, Expr? Default)> Params = [];
	}

	sealed class Emitter(Unit unit, IReadOnlyDictionary<string, int>? paramTypes, List<string> warnings)
	{
		readonly Dictionary<string, StructDef> structs = [];
		readonly Dictionary<string, List<FuncSig>> funcs = [];
		readonly List<Dictionary<string, Symbol>> scopes = [];
		readonly HashSet<string> helpers = [];
		readonly HashSet<string> assignedUniforms = [];
		readonly List<string> prologue = [];
		readonly Dictionary<string, int> constInts = [];
		readonly Dictionary<string, string> samplerUnits = [];
		readonly StringBuilder uniformsSb = new();
		bool secondPass;
		Ty currentRet = Ty.Void;

		void Warn(string msg)
		{
			if (!warnings.Contains(msg)) warnings.Add(msg);
		}

		// ---- naming / scopes

		static string SafeName(string n)
		{
			if (GlslReserved.Contains(n) || n.StartsWith("gl_", StringComparison.Ordinal)) n += "_";
			if (n.Contains("__")) n = n.Replace("__", "_x_");
			return n;
		}

		Symbol Declare(string name, Ty type)
		{
			var s = new Symbol { Glsl = SafeName(name), Type = type };
			scopes[^1][name] = s;
			return s;
		}

		Symbol? Lookup(string name)
		{
			for (int i = scopes.Count - 1; i >= 0; i--)
				if (scopes[i].TryGetValue(name, out var s)) return s;
			return null;
		}

		Ty Resolve(TypeRef t)
		{
			if (t.Resolved != null) return t.Resolved;
			if (structs.ContainsKey(t.Name)) return t.Resolved = new Ty { Kind = Kind.Struct, Struct = SafeName(t.Name) };
			var b = Parser.BaseType(t.Name) ?? throw new TranslateException($"unknown type '{t.Name}'");
			return t.Resolved = b;
		}

		Ty ResolveDecl(VarDecl d, Expr? init = null)
		{
			var t = Resolve(d.Type);
			if (d.Dims.Count == 0) return t;
			var dims = new List<int>();
			foreach (var de in d.Dims)
			{
				if (de == null) { dims.Add(-1); continue; }
				var v = ConstInt(de) ?? throw new TranslateException($"array size of '{d.Name}' is not a constant");
				dims.Add(v);
			}
			if (dims.Count == 1 && dims[0] < 0 && init is InitListExpr il)
				dims[0] = CountInitElements(il, t);
			return t.WithDims(dims);
		}

		int CountInitElements(InitListExpr il, Ty elem)
		{
			if (il.Items.All(i => i is InitListExpr) || elem.Kind == Kind.Struct) return il.Items.Count;
			int comps = 0;
			foreach (var it in Flatten(il)) comps += Math.Max(1, TypeOf(it).Comps);
			return Math.Max(1, comps / Math.Max(1, elem.Comps));
		}

		/// <summary>Evaluates a constant integer expression (array sizes).</summary>
		int? ConstInt(Expr e)
		{
			switch (e)
			{
				case LitExpr l when !l.IsBool:
					var txt = l.Text.TrimEnd('f', 'F', 'h', 'H', 'u', 'U', 'l', 'L');
					if (txt.StartsWith("0x", StringComparison.OrdinalIgnoreCase)) return (int)System.Convert.ToInt64(txt, 16);
					return (int)double.Parse(txt, CultureInfo.InvariantCulture);
				case ParenExpr pe: return ConstInt(pe.E);
				case CastExpr ce: return ConstInt(ce.E);
				case UnaryExpr { Op: "-" } u: return -ConstInt(u.E);
				case BinaryExpr b:
					var l2 = ConstInt(b.L); var r2 = ConstInt(b.R);
					if (l2 == null || r2 == null) return null;
					return b.Op switch { "+" => l2 + r2, "-" => l2 - r2, "*" => l2 * r2, "/" => r2 == 0 ? null : l2 / r2, _ => null };
				case IdentExpr id:
					return constInts.TryGetValue(id.Name, out var v) ? v : null;
			}
			return null;
		}

		// ---- run

		bool readsColor;

		public Result Run()
		{
			foreach (var item in unit.Items)
				if (item is StructDef sd) structs[sd.Name] = sd;

			string body = EmitAll();
			if (assignedUniforms.Count > 0)
			{
				// writable uniforms discovered: emit again so every reference uses the copy
				secondPass = true;
				body = EmitAll();
			}

			var common = new StringBuilder();
			common.AppendLine("in vec2 TexCoord;");
			common.AppendLine("out vec4 FragColor;");
			common.AppendLine("uniform vec4 uColor;");
			string help = EmitHelpers();
			if (help.Length > 0) common.AppendLine().Append(help);
			common.Append(body);

			const string banner = "// Auto-translated from DX9 HLSL by the Nuclear exporter.\n// Put a hand-written .frag in runtime/shaders/<api>/thirdparty to override.\n";
			string code = common.ToString().Replace("_uTint", readsColor ? "vec4(1.0)" : "uColor");
			return new Result
			{
				Gl330 = "#version 330 core\n" + banner + "\n" + code,
				Gles300 = "#version 300 es\nprecision highp float;\nprecision highp int;\n" + banner + "\n" + code,
				Warnings = warnings
			};
		}

		string EmitAll()
		{
			scopes.Clear();
			scopes.Add([]);
			funcs.Clear();
			prologue.Clear();
			constInts.Clear();
			uniformsSb.Clear();
			samplerUnits.Clear();

			var sb = new StringBuilder();
			foreach (var item in unit.Items)
			{
				switch (item)
				{
					case StructDef sd: sb.AppendLine().Append(EmitStruct(sd)); break;
					case List<VarDecl> decls: EmitGlobal(decls, sb); break;
					case FuncDef fd: sb.AppendLine().Append(EmitFunction(fd)); break;
				}
			}
			sb.AppendLine().Append(EmitMain());
			return uniformsSb + sb.ToString();
		}

		string EmitStruct(StructDef sd)
		{
			var sb = new StringBuilder();
			sb.AppendLine($"struct {SafeName(sd.Name)}");
			sb.AppendLine("{");
			foreach (var m in sd.Members)
			{
				var t = ResolveDecl(m);
				sb.AppendLine($"    {t.Glsl()} {SafeName(m.Name)}{t.ArraySuffix()};");
			}
			sb.AppendLine("};");
			return sb.ToString();
		}

		void EmitGlobal(List<VarDecl> decls, StringBuilder sb)
		{
			foreach (var d in decls)
			{
				var baseT = Resolve(d.Type);
				if (baseT.Kind == Kind.Sampler)
				{
					if (baseT.Struct == "texture") continue; // DX9 texture objects: the runtime binds textures to samplers directly
					if (d.Type.Name is "sampler3D" or "samplerCUBE") Warn($"{d.Type.Name} is not supported.");
					bool isImageParam = paramTypes?.TryGetValue(d.Name, out var pt0) == true && pt0 == 3;
					string glsl;
					if (d.Register == "s0" || (d.Register == "" && d.Name != "bkd" && !isImageParam && !samplerUnits.ContainsKey("uTexture")))
						glsl = "uTexture";
					else if (d.Register == "s1" || d.Name == "bkd")
						glsl = "bkd";
					else
					{
						glsl = SafeName(d.Name);
						Warn($"Sampler '{d.Name}' is not bound by the runtime (only s0 = object texture and s1 = background are).");
					}
					var sym = Declare(d.Name, Ty.Sampler);
					sym.Glsl = glsl;
					if (samplerUnits.TryAdd(glsl, d.Name))
						uniformsSb.AppendLine($"uniform sampler2D {glsl};");
					continue;
				}

				bool isParam = paramTypes?.ContainsKey(d.Name) == true || RuntimeUniforms.Contains(d.Name);
				bool hasInit = d.Init != null;
				// HLSL: non-static globals are uniforms. Fusion only sets its parameters, so an initialised
				// global that isn't a parameter keeps its initial value -> treat it as a constant/global.
				bool isUniform = !d.IsStatic && !(d.IsConst && hasInit) && (isParam || !hasInit || (paramTypes == null && d.Dims.Count == 0));
				var t = ResolveDecl(d, d.Init);

				if (isUniform)
				{
					EmitUniform(d, t);
					continue;
				}

				var s = Declare(d.Name, t);
				if (d.Init == null)
				{
					sb.AppendLine($"{t.Glsl()} {s.Glsl}{t.ArraySuffix()};");
					continue;
				}
				var init = EmitInit(d.Init, t);
				bool assigned = IsAssignedAnywhere(d.Name);
				if (init.IsConst)
				{
					bool makeConst = !assigned;
					if (makeConst && t.IsScalar && ConstInt(d.Init) is int ci) constInts[d.Name] = ci;
					s.IsConst = makeConst;
					sb.AppendLine($"{(makeConst ? "const " : "")}{t.Glsl()} {s.Glsl}{t.ArraySuffix()} = {init.Code};");
				}
				else
				{
					// GLSL requires constant global initializers: initialise at the start of main()
					sb.AppendLine($"{t.Glsl()} {s.Glsl}{t.ArraySuffix()};");
					prologue.Add($"{s.Glsl} = {init.Code};");
				}
			}
		}

		void EmitUniform(VarDecl d, Ty t)
		{
			var s = Declare(d.Name, t);
			s.IsUniform = true;
			if (!secondPass && s.Glsl != d.Name) Warn($"Parameter '{d.Name}' is a reserved word in GLSL and is declared as '{s.Glsl}'.");
			if (t.IsArray && t.Dims.Contains(-1)) throw new TranslateException($"uniform array '{d.Name}' has no size");

			// declare with the type the runtime uses to set it
			Ty storage = t;
			if (paramTypes != null && paramTypes.TryGetValue(d.Name, out var pt) && !t.IsArray)
			{
				storage = pt switch
				{
					0 => t.Kind == Kind.Bool && t.IsScalar ? Ty.Bool1 : Ty.Int1,
					1 => Ty.Float1,
					2 => Ty.Vec(4),
					_ => t
				};
			}
			else if (d.Type.Name is "int" or "uint" or "dword" && !t.IsArray)
				storage = Ty.Int1;
			if (!storage.SameAs(t)) s.Storage = storage;

			uniformsSb.AppendLine($"uniform {storage.Glsl()} {s.Glsl}{t.ArraySuffix()};");
			if (!secondPass && d.Init != null && !RuntimeUniforms.Contains(d.Name))
				Warn($"Default value of parameter '{d.Name}' ignored; the runtime sets it.");

			if (assignedUniforms.Contains(d.Name))
			{
				s.WritableCopy = $"{s.Glsl}_w";
				uniformsSb.AppendLine($"{t.Glsl()} {s.WritableCopy}{t.ArraySuffix()};");
				prologue.Add($"{s.WritableCopy} = {Convert(new Value(s.Glsl, storage), t).Code};");
			}
		}

		bool IsAssignedAnywhere(string name)
		{
			bool found = false;
			foreach (var item in unit.Items)
				if (item is FuncDef f && f.Body != null) Walk(f.Body, e =>
				{
					if (e is BinaryExpr b && IsAssignOp(b.Op) && RootName(b.L) == name) found = true;
					if (e is UnaryExpr u && u.Op is "++" or "--" && RootName(u.E) == name) found = true;
					if (e is CallExpr c && c.Args.Count >= 2 && c.Name is "sincos" or "modf" && c.Args.Skip(1).Any(a => RootName(a) == name)) found = true;
				});
			return found;
		}

		static bool IsAssignOp(string op) => op.EndsWith('=') && op is not ("==" or "!=" or "<=" or ">=");

		static string? RootName(Expr e) => e switch
		{
			IdentExpr i => i.Name,
			MemberExpr m => RootName(m.E),
			IndexExpr x => RootName(x.E),
			ParenExpr p => RootName(p.E),
			_ => null
		};

		static void Walk(Stmt s, Action<Expr> f)
		{
			switch (s)
			{
				case BlockStmt b: foreach (var x in b.Body) Walk(x, f); break;
				case DeclStmt d: foreach (var v in d.Decls) if (v.Init != null) Walk(v.Init, f); break;
				case ExprStmt e: if (e.E != null) Walk(e.E, f); break;
				case IfStmt i: Walk(i.C, f); Walk(i.T, f); if (i.F != null) Walk(i.F, f); break;
				case ForStmt fo: if (fo.Init != null) Walk(fo.Init, f); if (fo.C != null) Walk(fo.C, f); if (fo.Step != null) Walk(fo.Step, f); Walk(fo.Body, f); break;
				case WhileStmt w: Walk(w.C, f); Walk(w.Body, f); break;
				case ReturnStmt r: if (r.E != null) Walk(r.E, f); break;
			}
		}

		static void Walk(Expr e, Action<Expr> f)
		{
			f(e);
			switch (e)
			{
				case UnaryExpr u: Walk(u.E, f); break;
				case BinaryExpr b: Walk(b.L, f); Walk(b.R, f); break;
				case TernaryExpr t: Walk(t.C, f); Walk(t.T, f); Walk(t.F, f); break;
				case CallExpr c: foreach (var a in c.Args) Walk(a, f); if (c.Object != null) Walk(c.Object, f); break;
				case CastExpr c: Walk(c.E, f); break;
				case MemberExpr m: Walk(m.E, f); break;
				case IndexExpr i: Walk(i.E, f); Walk(i.I, f); break;
				case InitListExpr l: foreach (var x in l.Items) Walk(x, f); break;
				case ParenExpr p: Walk(p.E, f); break;
			}
		}

		string EmitFunction(FuncDef f)
		{
			var ret = Resolve(f.Ret);
			var sig = new FuncSig { Ret = ret, Glsl = SafeName(f.Name) };
			if (!funcs.TryGetValue(f.Name, out var list)) funcs[f.Name] = list = [];

			scopes.Add([]);
			samplerParamFlags.Clear();
			var ps = new List<string>();
			foreach (var p in f.Params)
			{
				var t = ResolveDecl(p);
				sig.Params.Add((t, p.ParamMod, p.Init));
				var s = Declare(p.Name, t);
				string mod = p.ParamMod == "in" ? "" : p.ParamMod + " ";
				ps.Add($"{mod}{t.Glsl()} {s.Glsl}{t.ArraySuffix()}");
				if (IsObjFlagged(t))
				{
					// hidden flag: is the sampler passed in the object texture? (see Tint)
					samplerParamFlags[s.Glsl] = $"isObj{s.Glsl.TrimStart('_')}_";
					ps.Add($"bool {samplerParamFlags[s.Glsl]}");
				}
			}
			if (!list.Any(o => o.Params.Count == sig.Params.Count && o.Params.Zip(sig.Params).All(z => z.First.Type.SameAs(z.Second.Type))))
				list.Add(sig);

			string header = $"{ret.Glsl()} {sig.Glsl}({string.Join(", ", ps)})";
			if (f.Body == null)
			{
				scopes.RemoveAt(scopes.Count - 1);
				return header + ";\n";
			}
			currentRet = ret;
			var sb = new StringBuilder();
			sb.AppendLine(header);
			EmitBlockBody(f.Body, sb, 0, newScope: false);
			scopes.RemoveAt(scopes.Count - 1);
			currentRet = Ty.Void;
			return sb.ToString();
		}

		// ---- main()

		string EmitMain()
		{
			var fdefs = unit.Items.OfType<FuncDef>().Where(f => f.Body != null).ToList();
			FuncDef? entry = unit.Entry != null ? fdefs.LastOrDefault(f => f.Name == unit.Entry) : null;
			if (entry == null)
			{
				entry = fdefs.LastOrDefault(f => f.Name == "ps_main") ?? fdefs.LastOrDefault(f => f.Name == "main")
					?? fdefs.LastOrDefault(f => f.RetSemantic.StartsWith("COLOR", StringComparison.Ordinal) || f.RetSemantic.StartsWith("SV_TARGET", StringComparison.Ordinal))
					?? throw new TranslateException("could not find the pixel shader entry point");
				if (!secondPass) Warn($"No technique found, using '{entry.Name}' as the entry point.");
			}

			var sb = new StringBuilder();
			sb.AppendLine("void main()");
			sb.AppendLine("{");
			foreach (var p in prologue) sb.AppendLine($"    {p}");
			readsColor = false;
			var args = new List<string>();
			string? colorOut = null;
			int n = 0;
			foreach (var p in entry.Params)
			{
				var t = ResolveDecl(p);
				string v = $"_in{n++}";
				if (p.IsUniform)
				{
					Warn($"Entry point uniform parameter '{p.Name}' is not supported; it reads as zero.");
					sb.AppendLine($"    {t.Glsl()} {v} = {Zero(t)};");
				}
				else if (p.ParamMod == "out")
				{
					sb.AppendLine($"    {t.Glsl()} {v};");
					if (p.Semantic.StartsWith("COLOR", StringComparison.Ordinal) || p.Semantic.StartsWith("SV_TARGET", StringComparison.Ordinal))
						colorOut ??= ToColor(new Value(v, t));
				}
				else if (t.Kind == Kind.Struct)
				{
					sb.AppendLine($"    {t.Glsl()} {v};");
					foreach (var m in structs[OriginalStructName(t.Struct!)].Members)
						sb.AppendLine($"    {v}.{SafeName(m.Name)} = {SemanticSource(m.Semantic, ResolveDecl(m), ref readsColor)};");
				}
				else
				{
					sb.AppendLine($"    {t.Glsl()} {v} = {SemanticSource(p.Semantic, t, ref readsColor)};");
				}
				args.Add(v);
			}

			var ret = Resolve(entry.Ret);
			string call = $"{SafeName(entry.Name)}({string.Join(", ", args)})";
			string color;
			if (ret.Kind == Kind.Struct)
			{
				var sd = structs[OriginalStructName(ret.Struct!)];
				var member = sd.Members.FirstOrDefault(m => m.Semantic.StartsWith("COLOR", StringComparison.Ordinal) || m.Semantic.StartsWith("SV_TARGET", StringComparison.Ordinal)) ?? sd.Members[0];
				sb.AppendLine($"    {ret.Glsl()} _out = {call};");
				color = ToColor(new Value($"_out.{SafeName(member.Name)}", ResolveDecl(member)));
			}
			else if (ret.Kind == Kind.Void)
			{
				sb.AppendLine($"    {call};");
				color = colorOut ?? "vec4(0.0)";
			}
			else color = ToColor(new Value(call, ret));

			// The RGB/blend coefficient is already in the object-texture samples (see Tint) or read via COLOR0.
			sb.AppendLine($"    FragColor = {color};");
			sb.AppendLine("}");
			return sb.ToString();
		}

		string ToColor(Value v)
		{
			if (v.Type.IsScalar) return $"vec4({Convert(v, Ty.Float1).Code})";
			if (v.Type.IsVector && v.Type.Cols == 3) return $"vec4({Convert(v, Ty.Vec(3)).Code}, 1.0)";
			if (v.Type.IsVector && v.Type.Cols == 2) return $"vec4({Convert(v, Ty.Vec(2)).Code}, 0.0, 1.0)";
			return Convert(v, Ty.Vec(4)).Code;
		}

		string OriginalStructName(string glsl) => structs.Keys.First(k => SafeName(k) == glsl);

		string SemanticSource(string semantic, Ty t, ref bool readsColor)
		{
			if (semantic.StartsWith("TEXCOORD", StringComparison.Ordinal))
				return Convert(new Value(t.Comps >= 3 ? "vec4(TexCoord, 0.0, 1.0)" : "TexCoord", t.Comps >= 3 ? Ty.Vec(4) : Ty.Vec(2)), t).Code;
			if (semantic.StartsWith("COLOR", StringComparison.Ordinal))
			{
				readsColor = true;
				return Convert(new Value("uColor", Ty.Vec(4)), t).Code;
			}
			if (semantic is "POSITION" or "POSITION0" or "SV_POSITION" or "VPOS")
				return Convert(new Value("gl_FragCoord", Ty.Vec(4)), t).Code;
			if (semantic.Length > 0 && !secondPass) Warn($"Input semantic {semantic} is not provided by the runtime; it reads as zero.");
			return Zero(t);
		}

		string Zero(Ty t)
		{
			if (t.IsArray)
			{
				int count = Math.Max(1, t.Dims.Aggregate(1, (a, b) => a * Math.Max(1, b)));
				return $"{t.Glsl()}[{count}]({string.Join(", ", Enumerable.Repeat(Zero(t.Element()), count))})";
			}
			if (t.Kind == Kind.Struct)
			{
				var sd = structs[OriginalStructName(t.Struct!)];
				return $"{t.Glsl()}({string.Join(", ", sd.Members.Select(m => Zero(ResolveDecl(m))))})";
			}
			if (t.Kind == Kind.Bool) return t.IsScalar ? "false" : $"{t.Glsl()}(false)";
			if (t.Kind == Kind.Int) return t.IsScalar ? "0" : $"{t.Glsl()}(0)";
			if (t.IsScalar || t.Kind == Kind.Unknown) return "0.0";
			return $"{t.Glsl()}(0.0)";
		}

		// ---- statements

		static string Ind(int n) => new(' ', n * 4);

		void EmitBlockBody(BlockStmt b, StringBuilder sb, int depth, bool newScope = true)
		{
			sb.AppendLine(Ind(depth) + "{");
			if (newScope) scopes.Add([]);
			foreach (var s in b.Body) EmitStmt(s, sb, depth + 1);
			if (newScope) scopes.RemoveAt(scopes.Count - 1);
			sb.AppendLine(Ind(depth) + "}");
		}

		void EmitStmt(Stmt s, StringBuilder sb, int depth)
		{
			string ind = Ind(depth);
			switch (s)
			{
				case BlockStmt b:
					EmitBlockBody(b, sb, depth);
					break;
				case DeclStmt d:
				{
					var code = EmitLocalDecl(d);
					if (code.Length > 0) sb.AppendLine(ind + code + ";");
					break;
				}
				case ExprStmt e:
					if (e.E != null) sb.AppendLine(ind + Emit(e.E).Code + ";");
					break;
				case IfStmt i:
					sb.AppendLine($"{ind}if ({ToBool(Emit(i.C)).Code})");
					EmitSub(i.T, sb, depth);
					if (i.F != null)
					{
						sb.AppendLine($"{ind}else");
						EmitSub(i.F, sb, depth);
					}
					break;
				case ForStmt f:
				{
					scopes.Add([]);
					string init = f.Init switch
					{
						DeclStmt d => EmitLocalDecl(d),
						ExprStmt { E: not null } es => Emit(es.E!).Code,
						_ => ""
					};
					string c = f.C != null ? ToBool(Emit(f.C)).Code : "";
					string step = f.Step != null ? Emit(f.Step).Code : "";
					sb.AppendLine($"{ind}for ({init}; {c}; {step})");
					EmitSub(f.Body, sb, depth);
					scopes.RemoveAt(scopes.Count - 1);
					break;
				}
				case WhileStmt w when !w.Do:
					sb.AppendLine($"{ind}while ({ToBool(Emit(w.C)).Code})");
					EmitSub(w.Body, sb, depth);
					break;
				case WhileStmt w:
					sb.AppendLine($"{ind}do");
					EmitSub(w.Body, sb, depth);
					sb.AppendLine($"{ind}while ({ToBool(Emit(w.C)).Code});");
					break;
				case ReturnStmt r:
					if (r.E == null) sb.AppendLine(ind + "return;");
					else if (currentRet.Kind == Kind.Void) sb.AppendLine($"{ind}{Emit(r.E).Code}; return;");
					else sb.AppendLine($"{ind}return {Convert(Emit(r.E), currentRet).Code};");
					break;
				case JumpStmt j:
					sb.AppendLine($"{ind}{j.Kind};");
					break;
			}
		}

		void EmitSub(Stmt s, StringBuilder sb, int depth)
		{
			if (s is BlockStmt b) { EmitBlockBody(b, sb, depth); return; }
			// wrap single statements in a block so declarations get their own scope
			sb.AppendLine(Ind(depth) + "{");
			scopes.Add([]);
			EmitStmt(s, sb, depth + 1);
			scopes.RemoveAt(scopes.Count - 1);
			sb.AppendLine(Ind(depth) + "}");
		}

		string EmitLocalDecl(DeclStmt ds)
		{
			var decls = new List<(Symbol sym, Ty t, Value? init)>();
			foreach (var d in ds.Decls)
			{
				var t = ResolveDecl(d, d.Init);
				if (t.Kind == Kind.Sampler)
				{
					// local sampler alias: "sampler2D s = Tex0;" -> the same uniform
					var alias = d.Init != null ? Emit(d.Init) : null;
					var sym2 = Declare(d.Name, Ty.Sampler);
					sym2.Glsl = alias?.Code ?? "uTexture";
					continue;
				}
				Value? init = d.Init != null ? EmitInit(d.Init, t) : null;
				var sym = Declare(d.Name, t); // declared after its initializer (HLSL scoping)
				decls.Add((sym, t, init));
			}
			if (decls.Count == 0) return "";
			// GLSL ES only allows 'const' with constant initializers
			bool isConst = ds.Decls.All(d => d.IsConst) && decls.All(d => d.init != null && d.init.IsConst);
			if (isConst)
			{
				foreach (var (sym, t, _) in decls) sym.IsConst = true;
				foreach (var d in ds.Decls)
					if (d.Init != null && ConstInt(d.Init) is int ci && Lookup(d.Name)?.Type.IsScalar == true) constInts[d.Name] = ci;
			}
			var parts = decls.Select(x => $"{x.sym.Glsl}{x.t.ArraySuffix()}{(x.init != null ? " = " + x.init.Code : "")}");
			return $"{(isConst ? "const " : "")}{decls[0].t.Glsl()} {string.Join(", ", parts)}";
		}

		// ---- initializers

		static IEnumerable<Expr> Flatten(InitListExpr il)
		{
			foreach (var it in il.Items)
			{
				if (it is InitListExpr sub) foreach (var x in Flatten(sub)) yield return x;
				else yield return it;
			}
		}

		Value EmitInit(Expr init, Ty t)
		{
			if (init is not InitListExpr il) return Convert(Emit(init), t);

			// HLSL brace initializers flatten everything into a stream of components and refill the target
			var leaves = Flatten(il).Select(Emit).ToList();
			bool isConst = leaves.All(l => l.IsConst);
			int pos = 0;
			string Fill(Ty target)
			{
				if (target.IsArray)
				{
					var elem = target.Element();
					int count = target.Dims.Aggregate(1, (a, b) => a * Math.Max(1, b));
					var items = new List<string>();
					for (int i = 0; i < count; i++) items.Add(Fill(elem));
					return $"{elem.Glsl()}[{count}]({string.Join(", ", items)})";
				}
				if (target.Kind == Kind.Struct)
				{
					var sd = structs[OriginalStructName(target.Struct!)];
					return $"{target.Glsl()}({string.Join(", ", sd.Members.Select(m => Fill(ResolveDecl(m))))})";
				}
				if (pos < leaves.Count && (leaves[pos].Type.SameAs(target) || (target.IsScalar && leaves[pos].Type.IsScalar)))
					return Convert(leaves[pos++], target).Code;
				int need = target.Comps, got = 0;
				var args = new List<string>();
				while (got < need && pos < leaves.Count)
				{
					var l = leaves[pos++];
					int c = Math.Max(1, l.Type.Comps);
					if (got + c > need)
					{
						args.Add(Convert(l, need - got == 1 ? Ty.Float1 : Ty.Vec(need - got)).Code); // leading components
						got = need;
						break;
					}
					args.Add(l.Type.Kind == Kind.Bool && target.Kind != Kind.Bool ? Convert(l, l.Type.WithKind(Kind.Float)).Code : l.Code);
					got += c;
				}
				if (args.Count == 0) return Zero(target);
				if (target.IsScalar) return Convert(new Value(args[0], Ty.Float1), target).Code;
				return $"{target.Glsl()}({string.Join(", ", args)})";
			}
			string code = Fill(t);
			if (pos < leaves.Count) Warn("Brace initializer has more values than the variable; extra values ignored.");
			return new Value(code, t, 100, isConst);
		}

		// ---- expressions

		static string FloatLit(string text)
		{
			string n = text;
			if (n.StartsWith("0x", StringComparison.OrdinalIgnoreCase))
				return System.Convert.ToInt64(n.TrimEnd('u', 'U', 'l', 'L'), 16).ToString(CultureInfo.InvariantCulture) + ".0";
			n = n.TrimEnd('f', 'F', 'h', 'H', 'u', 'U', 'l', 'L');
			if (n.EndsWith('.')) return n + "0";
			if (n.StartsWith('.')) n = "0" + n;
			if (n.Contains('.') || n.Contains('e') || n.Contains('E')) return n;
			return n.TrimStart('0').PadLeft(1, '0') + ".0";
		}

		Ty TypeOf(Expr e)
		{
			// type of an expression without keeping side effects
			var saved = helpers.ToHashSet();
			int w = warnings.Count;
			try { return Emit(e).Type; }
			catch { return Ty.Float1; }
			finally { helpers.Clear(); helpers.UnionWith(saved); if (warnings.Count > w) warnings.RemoveRange(w, warnings.Count - w); }
		}

		static string Wrap(Value v, int prec) => v.Prec < prec ? $"({v.Code})" : v.Code;

		static int BinPrec(string op) => op switch
		{
			"," => 1,
			"||" => 4, "&&" => 5, "|" => 6, "^" => 7, "&" => 8, "==" or "!=" => 9,
			"<" or ">" or "<=" or ">=" => 10, "<<" or ">>" => 11, "+" or "-" => 12, "*" or "/" or "%" => 13,
			_ => 2
		};

		Value Emit(Expr e)
		{
			switch (e)
			{
				case LitExpr l:
					if (l.IsBool) return new Value(l.Text, Ty.Bool1, 100, true);
					return new Value(FloatLit(l.Text), Ty.Float1, 100, true);
				case ParenExpr pe:
				{
					var v = Emit(pe.E);
					return new Value(v.Prec >= 100 ? v.Code : $"({v.Code})", v.Type, 100, v.IsConst);
				}
				case IdentExpr id: return EmitIdent(id);
				case UnaryExpr u: return EmitUnary(u);
				case BinaryExpr b: return EmitBinary(b);
				case TernaryExpr t: return EmitTernary(t);
				case CastExpr c: return EmitCast(c);
				case MemberExpr m: return EmitMember(m);
				case IndexExpr ix: return EmitIndex(ix);
				case CallExpr call: return EmitCall(call);
				case InitListExpr il:
				{
					var leaves = Flatten(il).Select(Emit).ToList();
					int comps = leaves.Sum(x => Math.Max(1, x.Type.Comps));
					var t = comps == 1 ? Ty.Float1 : Ty.Vec(Math.Min(comps, 4));
					return new Value($"{t.Glsl()}({string.Join(", ", leaves.Select(x => x.Code))})", t, 100, leaves.All(x => x.IsConst));
				}
			}
			throw new TranslateException("unsupported expression");
		}

		Value EmitIdent(IdentExpr id)
		{
			var s = Lookup(id.Name);
			if (s == null)
			{
				if (RuntimeUniforms.Contains(id.Name))
				{
					// Fusion provides these even if the shader forgot to declare them
					var sym = new Symbol { Glsl = id.Name, Type = Ty.Float1, IsUniform = true };
					scopes[0][id.Name] = sym;
					uniformsSb.AppendLine($"uniform float {id.Name};");
					return new Value(id.Name, Ty.Float1);
				}
				Warn($"Undeclared identifier '{id.Name}'.");
				return new Value(SafeName(id.Name), Ty.Unknown);
			}
			if (s.WritableCopy != null) return new Value(s.WritableCopy, s.Type);
			if (s.Storage != null) return Convert(new Value(s.Glsl, s.Storage), s.Type);
			return new Value(s.Glsl, s.Type, 100, s.IsConst);
		}

		void NoteAssign(Expr target)
		{
			var name = RootName(target);
			if (name == null) return;
			var s = Lookup(name);
			if (s != null && s.IsUniform && s.WritableCopy == null && !secondPass)
				assignedUniforms.Add(name);
		}

		Value EmitUnary(UnaryExpr u)
		{
			if (u.Op is "++" or "--") NoteAssign(u.E);
			var v = Emit(u.E);
			switch (u.Op)
			{
				case "++" or "--":
					return u.Postfix ? new Value($"{Wrap(v, 15)}{u.Op}", v.Type, 15) : new Value($"{u.Op}{Wrap(v, 14)}", v.Type, 14);
				case "!":
					if (v.Type.IsVector)
					{
						var fv = Convert(v, v.Type.WithKind(Kind.Float));
						return new Value($"equal({fv.Code}, {Zero(fv.Type)})", v.Type.WithKind(Kind.Bool), 100, v.IsConst);
					}
					return new Value($"!{Wrap(ToBool(v), 14)}", Ty.Bool1, 14, v.IsConst);
				case "~":
				{
					var ft = v.Type.IsVector ? v.Type.WithKind(Kind.Float) : Ty.Float1;
					string it = ft.IsScalar ? "int" : $"ivec{ft.Cols}";
					return new Value($"{ft.Glsl()}(~{it}({Convert(v, ft).Code}))", ft, 100, v.IsConst);
				}
				default:
				{
					if (v.Type.Kind == Kind.Bool) v = Convert(v, v.Type.WithKind(Kind.Float));
					string inner = Wrap(v, 14);
					if (inner.StartsWith('-') || inner.StartsWith('+')) inner = $"({inner})";
					return new Value($"{u.Op}{inner}", v.Type, 14, v.IsConst);
				}
			}
		}

		/// <summary>Integer bitwise operators: our ints live in floats, so go through int and back.</summary>
		Value IntOp(Value l, Value r, string op, bool cst)
		{
			var ct = Common(l.Type, r.Type).WithKind(Kind.Float);
			if (ct.IsMatrix || !ct.IsNumeric) ct = Ty.Float1;
			string it = ct.IsScalar ? "int" : $"ivec{ct.Cols}";
			return new Value($"{ct.Glsl()}({it}({Convert(l, ct).Code}) {op} {it}({Convert(r, ct).Code}))", ct, 100, cst);
		}

		/// <summary>Common type of two operands under HLSL promotion rules.</summary>
		static Ty Common(Ty a, Ty b, bool keepBool = false)
		{
			if (a.Kind == Kind.Unknown) return b.Kind == Kind.Unknown ? Ty.Float1 : b;
			if (b.Kind == Kind.Unknown) return a;
			if (a.Kind == Kind.Struct || a.IsArray) return a;
			if (b.Kind == Kind.Struct || b.IsArray) return b;
			Kind k = keepBool && a.Kind == Kind.Bool && b.Kind == Kind.Bool ? Kind.Bool : Kind.Float;
			if (a.IsMatrix || b.IsMatrix)
			{
				if (a.IsMatrix && b.IsMatrix) return Ty.Mat(Math.Min(a.Rows, b.Rows), Math.Min(a.Cols, b.Cols));
				return a.IsMatrix ? a : b;
			}
			int cols = a.Cols == 1 ? b.Cols : b.Cols == 1 ? a.Cols : Math.Min(a.Cols, b.Cols);
			return cols == 1 ? new Ty { Kind = k } : Ty.Vec(cols, k);
		}

		Value EmitBinary(BinaryExpr b)
		{
			string op = b.Op;
			if (op == ",")
			{
				var l0 = Emit(b.L); var r0 = Emit(b.R);
				return new Value($"{l0.Code}, {r0.Code}", r0.Type, 1);
			}

			if (IsAssignOp(op))
			{
				NoteAssign(b.L);
				var l = Emit(b.L);
				var r = Emit(b.R);
				if (op == "=")
					return new Value($"{l.Code} = {Convert(r, l.Type).Code}", l.Type, 2);
				string bop = op[..^1];
				if (bop == "%")
				{
					helpers.Add("fmod");
					return new Value($"{l.Code} = fmod({l.Code}, {Convert(r, l.Type).Code})", l.Type, 2);
				}
				if (bop is "&" or "|" or "^" or "<<" or ">>")
					return new Value($"{l.Code} = {IntOp(l, r, bop, false).Code}", l.Type, 2);
				if ((l.Type.IsVector || l.Type.IsMatrix) && r.Type.IsScalar && r.Type.Kind == Kind.Float)
					return new Value($"{l.Code} {op} {r.Code}", l.Type, 2);
				if (l.Type.IsMatrix && r.Type.IsMatrix && bop == "*")
					return new Value($"{l.Code} = matrixCompMult({l.Code}, {Convert(r, l.Type).Code})", l.Type, 2);
				return new Value($"{l.Code} {op} {Convert(r, l.Type).Code}", l.Type, 2);
			}

			var L = Emit(b.L);
			var R = Emit(b.R);
			bool cst = L.IsConst && R.IsConst;
			int prec = BinPrec(op);

			switch (op)
			{
				case "&&" or "||":
				{
					if (L.Type.IsVector || R.Type.IsVector)
					{
						var ct = Common(L.Type, R.Type).WithKind(Kind.Float);
						var lf = Convert(L, ct); var rf = Convert(R, ct);
						string comb = op == "&&" ? $"{Wrap(lf, 13)} * {Wrap(rf, 14)}" : $"abs({lf.Code}) + abs({rf.Code})";
						return new Value($"notEqual({comb}, {Zero(ct)})", ct.WithKind(Kind.Bool), 100, cst);
					}
					return new Value($"{Wrap(ToBool(L), prec)} {op} {Wrap(ToBool(R), prec + 1)}", Ty.Bool1, prec, cst);
				}
				case "==" or "!=" or "<" or ">" or "<=" or ">=":
				{
					var ct = Common(L.Type, R.Type, keepBool: op is "==" or "!=");
					if (ct.IsVector)
					{
						string fn = op switch { "==" => "equal", "!=" => "notEqual", "<" => "lessThan", ">" => "greaterThan", "<=" => "lessThanEqual", _ => "greaterThanEqual" };
						return new Value($"{fn}({Convert(L, ct).Code}, {Convert(R, ct).Code})", ct.WithKind(Kind.Bool), 100, cst);
					}
					if (ct.IsMatrix) ct = Ty.Float1;
					return new Value($"{Wrap(Convert(L, ct), prec)} {op} {Wrap(Convert(R, ct), prec + 1)}", Ty.Bool1, prec, cst);
				}
				case "%":
				{
					helpers.Add("fmod");
					var ct = Common(L.Type, R.Type).WithKind(Kind.Float);
					return new Value($"fmod({Convert(L, ct).Code}, {Convert(R, ct).Code})", ct, 100, false);
				}
				case "&" or "|" or "^" or "<<" or ">>":
					return IntOp(L, R, op, cst);
				default: // + - * /
				{
					if (L.Type.Kind == Kind.Bool) L = Convert(L, L.Type.WithKind(Kind.Float));
					if (R.Type.Kind == Kind.Bool) R = Convert(R, R.Type.WithKind(Kind.Float));
					var lt = L.Type; var rt = R.Type;
					if (lt.IsMatrix && rt.IsMatrix && op == "*")
					{
						var ct = Common(lt, rt);
						return new Value($"matrixCompMult({Convert(L, ct).Code}, {Convert(R, ct).Code})", ct, 100, cst);
					}
					if (lt.Kind == Kind.Unknown || rt.Kind == Kind.Unknown)
						return new Value($"{Wrap(L, prec)} {op} {Wrap(R, prec + 1)}", lt.Kind == Kind.Unknown ? rt : lt, prec, cst);
					if (lt.IsScalar || rt.IsScalar)
					{
						// GLSL handles vector op scalar natively
						var t = lt.IsScalar ? rt : lt;
						return new Value($"{Wrap(L, prec)} {op} {Wrap(R, prec + 1)}", t, prec, cst);
					}
					var c2 = Common(lt, rt);
					return new Value($"{Wrap(Convert(L, c2), prec)} {op} {Wrap(Convert(R, c2), prec + 1)}", c2, prec, cst);
				}
			}
		}

		Value EmitTernary(TernaryExpr t)
		{
			var c = Emit(t.C);
			var a = Emit(t.T);
			var b = Emit(t.F);
			bool cst = c.IsConst && a.IsConst && b.IsConst;
			if (c.Type.IsVector)
			{
				// component-wise select
				var rt = Common(Common(a.Type, b.Type), c.Type).WithKind(Kind.Float);
				var cb = Convert(c, rt.WithKind(Kind.Bool));
				return new Value($"mix({Convert(b, rt).Code}, {Convert(a, rt).Code}, {Convert(cb, rt).Code})", rt, 100, cst);
			}
			var ct = a.Type.Kind == Kind.Struct || a.Type.IsArray ? a.Type : Common(a.Type, b.Type, keepBool: true);
			return new Value($"{Wrap(ToBool(c), 4)} ? {Wrap(Convert(a, ct), 3)} : {Wrap(Convert(b, ct), 3)}", ct, 3, cst);
		}

		Value EmitCast(CastExpr c)
		{
			var v = Emit(c.E);
			var t = Resolve(c.T);
			if (t.Kind == Kind.Struct)
				return v.Type.IsScalar ? new Value(Zero(t), t, 100, true) : v;
			if (c.T.Name is "int" or "uint" or "dword" or "min16int" or "min12int" or "min16uint")
				return new Value($"trunc({Convert(v, Ty.Float1).Code})", Ty.Float1, 100, v.IsConst);
			var m = Regex.Match(c.T.Name, @"^(int|uint)([1-4])$");
			if (m.Success)
			{
				var vt = Ty.Vec(int.Parse(m.Groups[2].Value));
				return new Value($"trunc({Convert(v, vt, explicitCast: true).Code})", vt, 100, v.IsConst);
			}
			return Convert(v, t, explicitCast: true);
		}

		const string SwizzleChars = "xyzwrgbastpq";

		Value EmitMember(MemberExpr m)
		{
			var v = Emit(m.E);
			var t = v.Type;
			if (t.Kind == Kind.Struct)
			{
				var sd = structs[OriginalStructName(t.Struct!)];
				var mem = sd.Members.FirstOrDefault(x => x.Name == m.Name);
				if (mem == null) { Warn($"Struct {sd.Name} has no member '{m.Name}'."); return new Value($"{Wrap(v, 15)}.{SafeName(m.Name)}", Ty.Unknown, 15); }
				return new Value($"{Wrap(v, 15)}.{SafeName(mem.Name)}", ResolveDecl(mem), 15, v.IsConst);
			}
			if (t.IsMatrix && m.Name.StartsWith('_'))
			{
				var ms = Regex.Matches(m.Name, @"_m([0-3])([0-3])|_([1-4])([1-4])");
				var comps = ms.Select(x => x.Groups[1].Success
					? $"{Wrap(v, 15)}[{x.Groups[1].Value}][{x.Groups[2].Value}]"
					: $"{Wrap(v, 15)}[{int.Parse(x.Groups[3].Value) - 1}][{int.Parse(x.Groups[4].Value) - 1}]").ToList();
				if (comps.Count == 1) return new Value(comps[0], Ty.Float1, 15, v.IsConst);
				var vt = Ty.Vec(comps.Count);
				return new Value($"{vt.Glsl()}({string.Join(", ", comps)})", vt, 100, v.IsConst);
			}
			if (m.Name.Length <= 4 && m.Name.All(ch => SwizzleChars.Contains(ch)))
			{
				var k = t.Kind is Kind.Unknown or Kind.Int ? Kind.Float : t.Kind;
				var rt = m.Name.Length == 1 ? new Ty { Kind = k } : Ty.Vec(m.Name.Length, k);
				if (t.IsScalar) return rt.IsScalar ? v : new Value($"{rt.Glsl()}({v.Code})", rt, 100, v.IsConst);
				// normalise to xyzw so mixed sets (e.g. .ra from macros) stay valid
				string sw = new(m.Name.Select(ch => "xyzw"[SwizzleChars.IndexOf(ch) % 4]).ToArray());
				return new Value($"{Wrap(v, 15)}.{sw}", rt, 15, v.IsConst);
			}
			Warn($"Unknown member '.{m.Name}'.");
			return new Value($"{Wrap(v, 15)}.{SafeName(m.Name)}", Ty.Unknown, 15);
		}

		static string IndexCode(Value i) => Regex.IsMatch(i.Code, @"^\d+\.0$") ? i.Code[..^2] : $"int({i.Code})";

		Value EmitIndex(IndexExpr ix)
		{
			var v = Emit(ix.E);
			var i = Convert(Emit(ix.I), Ty.Float1);
			bool cst = v.IsConst && i.IsConst;

			if (v.RemainingDims != null)
			{
				string lin = $"({v.LinearIndex}) * {v.RemainingDims[0]}.0 + {Wrap(i, 13)}";
				var rest = v.RemainingDims.Skip(1).ToList();
				if (rest.Count == 0) return new Value($"{v.ArrayBase}[int({lin})]", v.Type.Element(), 15, cst);
				return new Value("", v.Type, 15, cst) { ArrayBase = v.ArrayBase, LinearIndex = lin, RemainingDims = rest };
			}
			var t = v.Type;
			if (t.IsArray)
			{
				if (t.Dims.Count > 1)
					return new Value("", t, 15, cst) { ArrayBase = Wrap(v, 15), LinearIndex = i.Code, RemainingDims = t.Dims.Skip(1).ToList() };
				return new Value($"{Wrap(v, 15)}[{IndexCode(i)}]", t.Element(), 15, cst);
			}
			if (t.IsMatrix) return new Value($"{Wrap(v, 15)}[{IndexCode(i)}]", Ty.Vec(t.Cols), 15, cst);
			if (t.IsVector) return new Value($"{Wrap(v, 15)}[{IndexCode(i)}]", new Ty { Kind = t.Kind == Kind.Int ? Kind.Float : t.Kind }, 15, cst);
			return new Value($"{Wrap(v, 15)}[{IndexCode(i)}]", Ty.Unknown, 15, cst);
		}

		// ---- conversions

		Value ToBool(Value v)
		{
			var t = v.Type;
			if (t.Kind == Kind.Bool && t.IsScalar) return v;
			if (t.Kind == Kind.Unknown) return new Value($"bool({v.Code})", Ty.Bool1, 100, v.IsConst);
			if (t.Kind == Kind.Bool) return new Value($"{Wrap(v, 15)}.x", Ty.Bool1, 15, v.IsConst);
			if (t.Kind == Kind.Int && t.IsScalar) return new Value($"{Wrap(v, 10)} != 0", Ty.Bool1, 9, v.IsConst);
			if (t.IsScalar) return new Value($"{Wrap(v, 10)} != 0.0", Ty.Bool1, 9, v.IsConst);
			if (t.IsVector) return new Value($"{Wrap(v, 15)}.x != 0.0", Ty.Bool1, 9, v.IsConst);
			return new Value($"bool({v.Code})", Ty.Bool1, 100, v.IsConst);
		}

		Value Convert(Value v, Ty to, bool explicitCast = false)
		{
			var from = v.Type;
			if (from.SameAs(to) || to.Kind == Kind.Unknown) return v;
			if (from.Kind == Kind.Unknown)
				return to.IsNumeric && !to.IsScalar ? new Value($"{to.Glsl()}({v.Code})", to, 100, v.IsConst) : v;
			if (to.Kind is Kind.Struct or Kind.Sampler or Kind.Void || from.Kind is Kind.Struct or Kind.Sampler or Kind.Void || to.IsArray || from.IsArray)
				return v;

			if (to.IsScalar)
			{
				Value s = from.IsScalar ? v
					: from.IsMatrix ? new Value($"{Wrap(v, 15)}[0][0]", Ty.Float1, 15, v.IsConst)
					: new Value($"{Wrap(v, 15)}.x", new Ty { Kind = from.Kind }, 15, v.IsConst);
				if (s.Type.Kind == to.Kind) return s;
				if (to.Kind == Kind.Bool) return ToBool(s);
				return new Value($"{to.Glsl()}({s.Code})", to, 100, v.IsConst);
			}

			if (to.IsMatrix)
			{
				if (from.IsScalar)
				{
					// HLSL splats a scalar to every component; GLSL mat(x) would build a diagonal
					var f = Convert(v, Ty.Float1);
					return new Value($"{to.Glsl()}({string.Join(", ", Enumerable.Repeat(f.Code, to.Rows * to.Cols))})", to, 100, v.IsConst);
				}
				return new Value($"{to.Glsl()}({v.Code})", to, 100, v.IsConst);
			}

			// to vector
			if (from.IsScalar)
			{
				var f = from.Kind == Kind.Int ? Convert(v, Ty.Float1) : v;
				return new Value($"{to.Glsl()}({f.Code})", to, 100, v.IsConst);
			}
			if (from.IsVector)
			{
				Value r = v;
				if (from.Cols > to.Cols)
					r = new Value($"{Wrap(v, 15)}.{"xyzw"[..to.Cols]}", Ty.Vec(to.Cols, from.Kind), 15, v.IsConst);
				else if (from.Cols < to.Cols)
				{
					var pad = string.Join(", ", Enumerable.Repeat(from.Kind == Kind.Bool ? "false" : from.Kind == Kind.Int ? "0" : "0.0", to.Cols - from.Cols));
					r = new Value($"{Ty.Vec(to.Cols, from.Kind).Glsl()}({v.Code}, {pad})", Ty.Vec(to.Cols, from.Kind), 100, v.IsConst);
				}
				if (r.Type.Kind != to.Kind) r = new Value($"{to.Glsl()}({r.Code})", to, 100, v.IsConst);
				return r;
			}
			if (from.IsMatrix)
			{
				if (to.Comps <= from.Cols) return Convert(new Value($"{Wrap(v, 15)}[0]", Ty.Vec(from.Cols), 15, v.IsConst), to);
				return new Value($"{to.Glsl()}({v.Code})", to, 100, v.IsConst);
			}
			return v;
		}

		static int ConvCost(Ty from, Ty to)
		{
			if (from.SameAs(to)) return 0;
			if (from.Kind == Kind.Unknown || to.Kind == Kind.Unknown) return 1;
			if (from.Kind == Kind.Struct || to.Kind == Kind.Struct) return from.Struct == to.Struct ? 0 : 1000;
			if (from.Kind == Kind.Sampler || to.Kind == Kind.Sampler) return from.Kind == to.Kind ? 0 : 1000;
			if (from.IsArray || to.IsArray) return from.Dims.SequenceEqual(to.Dims) ? 1 : 1000;
			int c = 0;
			if (from.Kind != to.Kind) c += 1;
			if (from.Comps != to.Comps) c += from.IsScalar ? 2 : from.Comps > to.Comps ? 3 : 50;
			if (from.IsMatrix != to.IsMatrix) c += 10;
			return c;
		}

		// ---- calls

		Value EmitCall(CallExpr call)
		{
			if (call.Object != null)
			{
				Warn($"Method call '.{call.Name}()' (DX10+ syntax) is not supported.");
				var args0 = call.Args.Select(Emit).ToList();
				return new Value($"{Emit(call.Object).Code}.{call.Name}({string.Join(", ", args0.Select(a => a.Code))})", Ty.Unknown);
			}
			if (call.Ctor != null) return EmitCtor(call);

			// user functions first (HLSL lets shaders overload intrinsic names)
			if (funcs.TryGetValue(call.Name, out var overloads))
			{
				var args = call.Args.Select(Emit).ToList();
				FuncSig? best = null;
				int bestCost = int.MaxValue;
				foreach (var o in overloads)
				{
					int required = o.Params.Count(p => p.Default == null);
					if (args.Count < required || args.Count > o.Params.Count) continue;
					int cost = 0;
					for (int i = 0; i < args.Count; i++) cost += ConvCost(args[i].Type, o.Params[i].Type);
					if (cost < bestCost) { best = o; bestCost = cost; }
				}
				if (best != null)
				{
					var codes = new List<string>();
					for (int i = 0; i < best.Params.Count; i++)
					{
						var (pt, mod, def) = best.Params[i];
						if (i < args.Count)
						{
							if (mod != "in") { NoteAssign(call.Args[i]); codes.Add(args[i].Code); }
							else codes.Add(Convert(args[i], pt).Code);
						}
						else codes.Add(Convert(Emit(def!), pt).Code);
						if (IsObjFlagged(pt))
						{
							string sc = i < args.Count ? args[i].Code : "";
							codes.Add(sc == "uTexture" ? "true" : samplerParamFlags.TryGetValue(sc, out var fl) ? fl : "false");
						}
					}
					return new Value($"{best.Glsl}({string.Join(", ", codes)})", best.Ret, 100, false);
				}
			}

			var r = EmitIntrinsic(call);
			if (r != null) return r;

			Warn($"Unknown function '{call.Name}'.");
			var a2 = call.Args.Select(Emit).ToList();
			return new Value($"{SafeName(call.Name)}({string.Join(", ", a2.Select(a => a.Code))})", Ty.Unknown);
		}

		Value EmitCtor(CallExpr call)
		{
			var t = Resolve(call.Ctor!);
			var args = call.Args.Select(Emit).ToList();
			bool cst = args.All(a => a.IsConst);
			if (t.Kind == Kind.Struct) return new Value($"{t.Glsl()}({string.Join(", ", args.Select(a => a.Code))})", t, 100, cst);
			bool isInt = Regex.IsMatch(call.Ctor!.Name, @"^(int|uint|dword)[1-4]?$");
			if (args.Count == 1)
			{
				var cv = Convert(args[0], t, explicitCast: true);
				return isInt ? new Value($"trunc({cv.Code})", t, 100, cst) : new Value(cv.Code, t, cv.Prec, cst);
			}
			var codes = new List<string>();
			int comps = 0;
			foreach (var a in args)
			{
				var at = a.Type;
				if (at.IsMatrix) { codes.Add(a.Code); comps += at.Comps; continue; }
				int c = Math.Max(1, at.Comps);
				// don't pass more components than the constructor needs (GLSL rejects unused arguments)
				if (comps + c > t.Comps && comps < t.Comps) { codes.Add(Convert(a, t.Comps - comps == 1 ? Ty.Float1 : Ty.Vec(t.Comps - comps)).Code); comps = t.Comps; continue; }
				if (comps >= t.Comps) continue;
				codes.Add(at.Kind is Kind.Bool or Kind.Int && t.Kind != at.Kind ? Convert(a, at.WithKind(Kind.Float)).Code : a.Code);
				comps += c;
			}
			string code = $"{t.Glsl()}({string.Join(", ", codes)})";
			if (isInt) code = $"trunc({code})";
			return new Value(code, t, 100, cst);
		}

		static readonly HashSet<string> SameShapeUnary = ["abs", "sin", "cos", "tan", "asin", "acos", "atan", "sinh", "cosh", "tanh", "exp", "exp2", "log", "log2", "sqrt", "ceil", "floor", "round", "trunc", "sign", "radians", "degrees", "normalize", "fwidth"];
		static readonly Dictionary<string, string> RenamedUnary = new() { ["frac"] = "fract", ["rsqrt"] = "inversesqrt", ["ddx"] = "dFdx", ["ddy"] = "dFdy", ["ddx_coarse"] = "dFdx", ["ddy_coarse"] = "dFdy", ["ddx_fine"] = "dFdx", ["ddy_fine"] = "dFdy" };
		static readonly Dictionary<string, string> PromotedMulti = new() { ["pow"] = "pow", ["atan2"] = "atan", ["min"] = "min", ["max"] = "max", ["step"] = "step", ["lerp"] = "mix", ["clamp"] = "clamp", ["smoothstep"] = "smoothstep", ["fmod"] = "fmod", ["reflect"] = "reflect", ["faceforward"] = "faceforward" };

		Value? EmitIntrinsic(CallExpr call)
		{
			string name = call.Name;
			var args = call.Args.Select(Emit).ToList();
			bool cst = args.All(a => a.IsConst);
			Ty Shape(IEnumerable<Value> vs)
			{
				Ty? acc = null;
				foreach (var v in vs) acc = acc == null ? v.Type : Common(acc, v.Type);
				return acc == null || acc.Kind is Kind.Unknown or Kind.Struct or Kind.Sampler || acc.IsArray ? Ty.Float1 : acc.WithKind(Kind.Float);
			}
			string A(int i, Ty t) => Convert(args[i], t).Code;

			if ((SameShapeUnary.Contains(name) || RenamedUnary.ContainsKey(name)) && args.Count == 1)
			{
				var t = Shape(args);
				string fn = RenamedUnary.GetValueOrDefault(name, name);
				return new Value($"{fn}({A(0, t)})", t, 100, cst && !fn.StartsWith("dF", StringComparison.Ordinal) && fn != "fwidth");
			}
			if (name == "saturate" && args.Count == 1)
			{
				var t = Shape(args);
				return new Value($"clamp({A(0, t)}, 0.0, 1.0)", t, 100, cst);
			}
			if (name == "rcp" && args.Count == 1)
			{
				var t = Shape(args);
				return new Value($"(1.0 / {Wrap(Convert(args[0], t), 14)})", t, 100, cst);
			}
			if (name == "log10" && args.Count == 1)
			{
				var t = Shape(args);
				return new Value($"(log({A(0, t)}) * 0.4342944819)", t, 100, cst);
			}
			if (name is "isnan" or "isinf" or "isfinite" && args.Count == 1)
			{
				var t = Shape(args);
				var bt = t.WithKind(Kind.Bool);
				if (name == "isfinite")
					return new Value(t.IsScalar ? $"!(isnan({A(0, t)}) || isinf({A(0, t)}))" : $"not(bvec{t.Cols}(vec{t.Cols}(isnan({A(0, t)})) + vec{t.Cols}(isinf({A(0, t)}))))", bt, 100, cst);
				return new Value($"{name}({A(0, t)})", bt, 100, cst);
			}
			if (PromotedMulti.TryGetValue(name, out var gl) && args.Count >= 2)
			{
				var t = Shape(args);
				if (name == "fmod") { helpers.Add("fmod"); cst = false; }
				return new Value($"{gl}({string.Join(", ", args.Select((_, i) => A(i, t)))})", t, 100, cst);
			}
			if (name == "refract" && args.Count == 3)
			{
				var t = Shape(args.Take(2));
				return new Value($"refract({A(0, t)}, {A(1, t)}, {A(2, Ty.Float1)})", t, 100, cst);
			}
			if (name == "mad" && args.Count == 3)
			{
				var t = Shape(args);
				return new Value($"({Wrap(Convert(args[0], t), 13)} * {Wrap(Convert(args[1], t), 14)} + {Wrap(Convert(args[2], t), 13)})", t, 100, cst);
			}
			if (name == "ldexp" && args.Count == 2)
			{
				var t = Shape(args);
				return new Value($"({Wrap(Convert(args[0], t), 13)} * exp2({A(1, t)}))", t, 100, cst);
			}
			if (name is "dot" or "distance" && args.Count == 2)
			{
				var t = Shape(args);
				if (t.IsScalar) return name == "dot" ? new Value($"({Wrap(Convert(args[0], t), 13)} * {Wrap(Convert(args[1], t), 14)})", Ty.Float1, 100, cst) : new Value($"abs({A(0, t)} - {A(1, t)})", Ty.Float1, 100, cst);
				return new Value($"{name}({A(0, t)}, {A(1, t)})", Ty.Float1, 100, cst);
			}
			if (name == "length" && args.Count == 1)
			{
				var t = Shape(args);
				return new Value($"length({A(0, t)})", Ty.Float1, 100, cst);
			}
			if (name == "cross" && args.Count == 2)
				return new Value($"cross({A(0, Ty.Vec(3))}, {A(1, Ty.Vec(3))})", Ty.Vec(3), 100, cst);
			if (name is "any" or "all" && args.Count == 1)
			{
				var t = args[0].Type;
				if (t.Kind == Kind.Bool && t.IsVector) return new Value($"{name}({args[0].Code})", Ty.Bool1, 100, cst);
				if (t.IsVector)
				{
					var ft = t.WithKind(Kind.Float);
					return new Value($"{name}(notEqual({A(0, ft)}, {Zero(ft)}))", Ty.Bool1, 100, cst);
				}
				return ToBool(args[0]);
			}
			if (name == "mul" && args.Count == 2)
			{
				var a = args[0]; var b = args[1];
				var at = a.Type; var bt = b.Type;
				if (at.IsScalar || bt.IsScalar) return new Value($"({Wrap(a, 13)} * {Wrap(b, 14)})", at.IsScalar ? bt : at, 100, cst);
				Ty rt;
				if (at.IsVector && bt.IsMatrix) { rt = Ty.Vec(bt.Cols); a = Convert(a, Ty.Vec(bt.Rows)); }
				else if (at.IsMatrix && bt.IsVector) { rt = Ty.Vec(at.Rows); b = Convert(b, Ty.Vec(at.Cols)); }
				else if (at.IsMatrix && bt.IsMatrix) rt = Ty.Mat(at.Rows, bt.Cols);
				else
				{
					var t = Shape(args);
					return new Value($"dot({A(0, t)}, {A(1, t)})", Ty.Float1, 100, cst);
				}
				// matrices are stored transposed, so mul(a, b) == b * a
				return new Value($"({Wrap(b, 13)} * {Wrap(a, 14)})", rt, 100, cst);
			}
			if (name == "transpose" && args.Count == 1)
			{
				var t = args[0].Type;
				return new Value($"transpose({args[0].Code})", t.IsMatrix ? Ty.Mat(t.Cols, t.Rows) : t, 100, cst);
			}
			if (name == "determinant" && args.Count == 1)
				return new Value($"determinant({args[0].Code})", Ty.Float1, 100, cst);
			if (name == "clip" && args.Count == 1)
			{
				var t = Shape(args);
				return new Value(t.IsScalar ? $"if ({Wrap(Convert(args[0], t), 10)} < 0.0) discard" : $"if (any(lessThan({A(0, t)}, {Zero(t)}))) discard", Ty.Void, 0);
			}
			if (name == "sincos" && args.Count == 3)
			{
				var t = Shape(args.Take(1));
				helpers.Add("sincos");
				NoteAssign(call.Args[1]); NoteAssign(call.Args[2]);
				return new Value($"sincos({A(0, t)}, {args[1].Code}, {args[2].Code})", Ty.Void, 100);
			}
			if (name == "modf" && args.Count == 2)
			{
				var t = Shape(args.Take(1));
				NoteAssign(call.Args[1]);
				return new Value($"modf({A(0, t)}, {args[1].Code})", t, 100);
			}
			if (name is "tex2D" or "tex2Dlod" or "tex2Dbias" or "tex2Dproj" or "tex2Dgrad" && args.Count >= 2)
			{
				string s = args[0].Code;
				if (args[0].Type.Kind != Kind.Sampler) Warn($"{name} called on a non-sampler.");
				// D3D samples non-finite coordinates (e.g. uv / 0) at texel (0,0); GL's result is undefined, so sanitise them with _tc().
				helpers.Add("tc");
				return Tint(s, name switch
				{
					"tex2D" when args.Count == 4 => new Value($"textureGrad({s}, _tc({s}, {A(1, Ty.Vec(2))}), {A(2, Ty.Vec(2))}, {A(3, Ty.Vec(2))})", Ty.Vec(4)),
					"tex2D" => new Value($"texture({s}, _tc({s}, {A(1, Ty.Vec(2))}))", Ty.Vec(4)),
					"tex2Dlod" => TexXyW(s, args[1], "tex2Dlod"),
					"tex2Dbias" => TexXyW(s, args[1], "tex2Dbias"),
					"tex2Dproj" => new Value($"textureProj({s}, _tc({A(1, Ty.Vec(4))}))", Ty.Vec(4)),
					_ when args.Count == 4 => new Value($"textureGrad({s}, _tc({s}, {A(1, Ty.Vec(2))}), {A(2, Ty.Vec(2))}, {A(3, Ty.Vec(2))})", Ty.Vec(4)),
					_ => new Value($"texture({s}, _tc({s}, {A(1, Ty.Vec(2))}))", Ty.Vec(4)),
				});
			}
			if (name == "tex1D" && args.Count == 2)
			{
				helpers.Add("tc");
				return Tint(args[0].Code, new Value($"texture({args[0].Code}, _tc({args[0].Code}, vec2({A(1, Ty.Float1)}, 0.5)))", Ty.Vec(4)));
			}
			if (name is "tex3D" or "texCUBE" or "noise" or "lit" or "asfloat" or "asint" or "asuint" or "f16tof32" or "f32tof16" or "D3DCOLORtoUBYTE4")
			{
				Warn($"Intrinsic '{name}' has no GLSL translation.");
				return new Value($"{name}({string.Join(", ", args.Select(a => a.Code))})", Ty.Vec(4));
			}
			return null;
		}

		// Fusion's DX9 runtime applies the object's RGB/blend coefficient to its image before the shader runs
		// (unless the shader takes the coefficient itself via COLOR0), so tint samples of the object texture,
		// including those made through a sampler parameter that receives it. _uTint is resolved in Run() once
		// we know whether COLOR0 is read.
		Value Tint(string sampler, Value v)
		{
			if (sampler == "uTexture")
			{
				return new Value($"({v.Code} * _uTint)", Ty.Vec(4));
			}
			if (samplerParamFlags.TryGetValue(sampler, out var flag))
			{
				return new Value($"({v.Code} * ({flag} ? _uTint : vec4(1.0)))", Ty.Vec(4));
			}
			return v;
		}

		// sampler parameters of user functions (glsl name -> hidden "is the object texture" flag)
		readonly Dictionary<string, string> samplerParamFlags = [];
		static bool IsObjFlagged(Ty t) => t.Kind == Kind.Sampler && !t.IsArray;

		Value TexXyW(string sampler, Value uv, string helper)
		{
			helpers.Add(helper);
			return new Value($"_{helper}({sampler}, {Convert(uv, Ty.Vec(4)).Code})", Ty.Vec(4));
		}

		string EmitHelpers()
		{
			var sb = new StringBuilder();
			if (helpers.Contains("fmod"))
				foreach (var t in new[] { "float", "vec2", "vec3", "vec4" })
					sb.AppendLine($"{t} fmod({t} x, {t} y) {{ return x - y * trunc(x / y); }}");
			if (helpers.Contains("sincos"))
				foreach (var t in new[] { "float", "vec2", "vec3", "vec4" })
					sb.AppendLine($"void sincos({t} x, out {t} s, out {t} c) {{ s = sin(x); c = cos(x); }}");
			if (helpers.Contains("tc"))
			{
				// NaN/Inf -> 0. Tested on the bits so fast-math can't fold it away; mix() with a bvec selects, so NaN doesn't leak through.
				sb.AppendLine("vec2 _tc(sampler2D s, vec2 c) { return mix(c, 0.5 / vec2(textureSize(s, 0)), equal(floatBitsToUint(c) & 0x7f800000u, uvec2(0x7f800000u))); }");
				sb.AppendLine("vec4 _tc(vec4 c) { return mix(c, vec4(0.0), equal(floatBitsToUint(c) & 0x7f800000u, uvec4(0x7f800000u))); }");
			}
			if (helpers.Contains("tex2Dlod")) sb.AppendLine("vec4 _tex2Dlod(sampler2D s, vec4 uv) { return textureLod(s, _tc(s, uv.xy), uv.w); }");
			if (helpers.Contains("tex2Dbias")) sb.AppendLine("vec4 _tex2Dbias(sampler2D s, vec4 uv) { return texture(s, _tc(s, uv.xy), uv.w); }");
			return sb.ToString();
		}
	}
}
