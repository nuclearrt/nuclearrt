using System.Text;
using CTFAK.Utils;

public static class StringUtils
{
	public static string SanitizeString(string input)
	{
		if (string.IsNullOrEmpty(input))
		{
			return "";
		}

		var result = new StringBuilder();
		foreach (char c in input)
		{
			switch (c)
			{
				case '"':
					result.Append("\\\"");
					break;
				case '\\':
					result.Append("\\\\");
					break;
				case '\r':
					result.Append("\\r");
					break;
				case '\n':
					result.Append("\\n");
					break;
				case '\t':
					result.Append("\\t");
					break;
				case '\b':
					result.Append("\\b");
					break;
				case '\f':
					result.Append("\\f");
					break;
				case '\v':
					result.Append("\\v");
					break;
				case '\a':
					result.Append("\\a");
					break;
				case '\0':
					result.Append("\\0");
					break;
				default:
					if (c < 32 || c > 126)
					{
						result.Append($"\\u{(int)c:X4}");
					}
					else
					{
						result.Append(c);
					}
					break;
			}
		}
		return result.ToString();
	}

	public static string SanitizeObjectName(string input)
	{
		if (string.IsNullOrEmpty(input))
		{
			Logger.Log($"SanitizeObjectName: input is null or empty");
			return "";
		}

		// if the first character is a number, add an underscore
		if (char.IsDigit(input[0]))
		{
			input = "_" + input;
		}
		
		string[] invalidChars = [" ", ".", "-", ":", ";", ",", "!", "?", "*", "/", "\\", "|", "`", "'", "\"", "&", "+", "(", ")", "=", "%", "[", "]", "@"];
		
		foreach (string invalidChar in invalidChars)
		{
			input = input.Replace(invalidChar, "_");
		}
		
		return SanitizeString(input).Replace("\\", "_");
	}
}
