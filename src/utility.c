//---------------------------------------------------------------------------
// utility.c
//
// Miscellaneous utility functions.
//
// Subdue 5
//---------------------------------------------------------------------------

#include "subdue.h"


//---------------------------------------------------------------------------
// NAME: OutOfMemoryError
//
// INPUTS: (char *context)
//
// RETURN: (void)
//
// PURPOSE: Print out of memory error with context, and then exit.
//---------------------------------------------------------------------------

void OutOfMemoryError(char *context)
{
   printf("ERROR: out of memory allocating %s.\n", context);
   exit(1);
}


//---------------------------------------------------------------------------
// NAME: PrintBoolean
//
// INPUTS: (BOOLEAN boolean)
//
// RETURN: (void)
//
// PURPOSE: Print BOOLEAN input as 'TRUE' or 'FALSE'.
//---------------------------------------------------------------------------

void PrintBoolean(BOOLEAN boolean)
{
   if (boolean)
      printf("true\n");
   else 
      printf("false\n");
}
