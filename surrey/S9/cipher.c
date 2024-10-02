/*
 * Simple implementation of Caesar cipher.
 * Only support small letters.
 */

/* encryption */
void encrypt(unsigned char *input, unsigned char *output, int shiftKey)
{
  int i;
  int shift = shiftKey%26;
  for (i=0; i<strlen(input); i++)
  {
    if (input[i]>='a' && input[i]<='z')
    {
      output[i] = input[i] + shift;
      if (output[i]>'z') output[i]-=26;
    }
    else output[i] = input[i];
  }
  output[i] = 0;
}

/* decryption */
void decrypt(unsigned char *input, unsigned char *output, int shiftKey)
{
  int i;
  int shift = shiftKey%26;
  for (i=0; i<strlen(input); i++)
  {
    if (input[i]>='a' && input[i]<='z')
    {
      output[i] = input[i] - shift;
      if (output[i]<'a') output[i]+=26;
    }
    else output[i] = input[i];
  }
  output[i] = 0;
}

