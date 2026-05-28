using System;

class KMP
{
    // Funkcja budująca tablicę LPS (Longest Prefix Suffix)
    static int[] BuildLPS(string pattern)
    {
        int[] lps = new int[pattern.Length];
        int length = 0;
        int i = 1;

        while (i < pattern.Length)
        {
            if (pattern[i] == pattern[length])
            {
                length++;
                lps[i] = length;
                i++;
            }
            else
            {
                if (length != 0)
                {
                    length = lps[length - 1];
                }
                else
                {
                    lps[i] = 0;
                    i++;
                }
            }
        }

        return lps;
    }

    // Algorytm KMP
    static void KMPSearch(string text, string pattern)
    {
        int[] lps = BuildLPS(pattern);

        int i = 0; // indeks tekstu
        int j = 0; // indeks wzorca

        while (i < text.Length)
        {
            if (pattern[j] == text[i])
            {
                i++;
                j++;
            }

            // znaleziono wzorzec
            if (j == pattern.Length)
            {
                Console.WriteLine("Znaleziono wzorzec na pozycji: " + (i - j));
                j = lps[j - 1];
            }
            else if (i < text.Length && pattern[j] != text[i])
            {
                if (j != 0)
                {
                    j = lps[j - 1];
                }
                else
                {
                    i++;
                }
            }
        }
    }

    static void Main()
    {
        string text = "ABABDABACDABABCABAB";
        string pattern = "ABABCABAB";

        KMPSearch(text, pattern);
    }
}
