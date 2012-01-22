//---------------------------------------------------------------------------
// labels.c
//
// Storage and lookup of graph vertex and edge labels.
//
// Subdue 5
//---------------------------------------------------------------------------

#include "subdue.h"


//---------------------------------------------------------------------------
// NAME:    AllocateLabelList
//
// INPUTS:  (void)
//
// RETURN:  (LabelList *)
//
// PURPOSE: Allocate memory for empty label list.
//---------------------------------------------------------------------------

LabelList *AllocateLabelList(void)
{
   LabelList *labelList;
   labelList = (LabelList *) malloc(sizeof(LabelList));
   if (labelList == NULL)
      OutOfMemoryError("AllocateLabelList:labelList");
   labelList->size = 0;
   labelList->numLabels = 0;
   labelList->labels = NULL;
   return labelList;
}


//---------------------------------------------------------------------------
// NAME:    StoreLabel
//
// INPUTS:  (Label *label) - label to be stored (if necessary)
//          (LabelList *labelList) - list to contain label
//
// RETURN:  (ULONG) - label's index in given label list
//
// PURPOSE: Stores the given label, if not already present, in the
// given label list and returns the label's index.  The given label's
// memory can be freed after executing StoreLabel.
//---------------------------------------------------------------------------

ULONG StoreLabel(Label *label, LabelList *labelList)
{
   ULONG labelIndex;
   Label *newLabelList;
   char *stringLabel;

   labelIndex = GetLabelIndex(label, labelList);
   if (labelIndex == labelList->numLabels) 
   { // i.e., label not found
      // make sure there is room for a new label
      if (labelList->size == labelList->numLabels) 
      {
         labelList->size += LIST_SIZE_INC;
         newLabelList = (Label *) realloc(labelList->labels,
                                          (sizeof(Label) * labelList->size));
         if (newLabelList == NULL)
            OutOfMemoryError("StoreLabel:newLabelList");
         labelList->labels = newLabelList;
      }
      // store label
      labelList->labels[labelList->numLabels].labelType = label->labelType;
      switch(label->labelType) 
      {
         case STRING_LABEL:
            stringLabel = (char *) malloc
                          (sizeof(char) * 
                           (strlen(label->labelValue.stringLabel)) + 1);
            if (stringLabel == NULL)
               OutOfMemoryError("StoreLabel:stringLabel");
            strcpy(stringLabel, label->labelValue.stringLabel);
            labelList->labels[labelList->numLabels].labelValue.stringLabel =
               stringLabel;
            break;
         case NUMERIC_LABEL:
            labelList->labels[labelList->numLabels].labelValue.numericLabel =
               label->labelValue.numericLabel;
            break;
         default:
            break;  // error
      }
      labelList->labels[labelList->numLabels].used = FALSE;
      labelList->numLabels++;
   }
   return labelIndex;
}


//---------------------------------------------------------------------------
// NAME:    GetLabelIndex
//
// INPUTS:  (Label *label) - label being sought
//          (LabelList *labelList) - list in which to look for label
//
// RETURN: (ULONG) - index of label in label list, or number of labels if
//                   label not found
//
// PURPOSE: Returns the index of the given label in the given label
// list.  If not found, then the index just past the end (i.e., number
// of stored labels) is returned.
//---------------------------------------------------------------------------

ULONG GetLabelIndex(Label *label, LabelList *labelList)
{
   ULONG i = 0;
   ULONG labelIndex = labelList->numLabels;
   BOOLEAN found = FALSE;

   while ((i < labelList->numLabels) && (! found)) 
   {
      if (labelList->labels[i].labelType == label->labelType) 
      {
         switch(label->labelType) 
         {
            case STRING_LABEL:
               if (strcmp(labelList->labels[i].labelValue.stringLabel,
                   label->labelValue.stringLabel) == 0) 
               {
                  labelIndex = i;
                  found = TRUE;
               }
               break;
            case NUMERIC_LABEL:
               if (labelList->labels[i].labelValue.numericLabel ==
                   label->labelValue.numericLabel) 
               {
                  labelIndex = i;
                  found = TRUE;
               }
            default:
               break;  // error
         }
      }
      i++;
   }
   return labelIndex;
}


//---------------------------------------------------------------------------
// NAME: SubLabelNumber
//
// INPUTS: (ULONG index) - index of label in labelList
//         (LabelList *labelList) - list of labels
//
// RETURN: (ULONG) - number from substructure label, or zero if 
//                   label is not a valid substructure label
//
// PURPOSE: Checks if label is a valid substructure label of the form
// <SUB_LABEL_STRING>_<#>, where <#> is greater than zero.  If valid,
// then <#> is returned; otherwise, returns zero.
//---------------------------------------------------------------------------

ULONG SubLabelNumber(ULONG index, LabelList *labelList)
{
   char *stringLabel;
   char prefix[TOKEN_LEN];
   char rest[TOKEN_LEN];
   BOOLEAN match;
   int i = 0;
   int labelLength;
   int prefixLength;
   ULONG subNumber;

   match = TRUE;
   subNumber = 0;
   if (labelList->labels[index].labelType == STRING_LABEL) 
   { // string label?
      stringLabel = labelList->labels[index].labelValue.stringLabel;
      labelLength = strlen(stringLabel);
      strcpy(prefix, SUB_LABEL_STRING);
      prefixLength = strlen(prefix);
      // check that first part of label matches SUB_LABEL_STRING
      if (labelLength > (prefixLength + 1))
         for (i = 0; ((i < prefixLength) && match); i++)
            if (stringLabel[i] != prefix[i])
               match = FALSE;
      if (match && (stringLabel[i] != '_')) // underscore present?
         match = FALSE;
      if (match &&                          // rest is a number?
          (sscanf((& stringLabel[i + 1]), "%lu%s", &subNumber, rest) != 1))
         subNumber = 0;
   }
   return subNumber;
}


//---------------------------------------------------------------------------
// NAME: LabelMatchFactor
//
// INPUTS: (ULONG labelIndex1)
//         (ULONG labelIndex2) - indices of labels to match
//         (LabelList *labelList) - list of labels
//
// RETURN: (double) - degree of mis-match [0.0,1.0]
//
// PURPOSE: Return the degree of mis-match between the two labels.  If
// the label indices are equal, then the degree of mis-match is 0.0,
// else 1.0.  Values between 0.0 and 1.0 may be implemented based on
// specifics of labels (e.g., threshold or tolerance matching for
// numeric labels, or semantic distance for string labels).
//---------------------------------------------------------------------------

double LabelMatchFactor(ULONG labelIndex1, ULONG labelIndex2,
          LabelList *labelList)
{
   if (labelIndex1 == labelIndex2)
      return 0.0;

   // ***** May want more elaborate matching scheme here, i.e., the
   // ***** tolerance or threshold matching of numeric labels.

   return 1.0;
}


//---------------------------------------------------------------------------
// NAME:    PrintLabel
//
// INPUTS:  (ULONG index) - index into label list
//          (LabelList *labelList) - list of labels
//
// RETURN:  void
//
// PURPOSE: Print label of given index.
//---------------------------------------------------------------------------

void PrintLabel(ULONG index, LabelList *labelList)
{
   UCHAR labelType;

   labelType = labelList->labels[index].labelType;
   switch(labelType) 
   {
      case STRING_LABEL:
         fprintf(stdout, "%s", labelList->labels[index].labelValue.stringLabel);
         break;
      case NUMERIC_LABEL:
         fprintf(stdout, "%.*g", NUMERIC_OUTPUT_PRECISION,
                 labelList->labels[index].labelValue.numericLabel);
         break;
      default:
         break;
   }
}


//---------------------------------------------------------------------------
// NAME:    PrintLabelList
//
// INPUTS:  (LabelList *labelList) - list of labels
//
// RETURN:  void
//
// PURPOSE: Print labels in given labelList.
//---------------------------------------------------------------------------

void PrintLabelList(LabelList *labelList)
{
   ULONG i;

   printf("Label list:\n");
   for (i = 0; i < labelList->numLabels; i++) 
   {
      printf("  labels[%lu] = ", i);
      PrintLabel(i, labelList);
      printf("\n");
   }
}


//---------------------------------------------------------------------------
// NAME: FreeLabelList
//
// INPUTS: (LabelList *labelList)
//
// RETURN: (void)
//
// PURPOSE: Free memory in labelList.
//---------------------------------------------------------------------------

void FreeLabelList(LabelList *labelList)
{
   free(labelList->labels);
   free(labelList);
}


//---------------------------------------------------------------------------
// NAME: WriteLabelToFile
//
// INPUTS: (FILE *outFile) - file to write to
//         (ULONG index) - index of label in label list
//         (LabelList *labelList) - label list
//         (BOOLEAN suppressQuotes) - if TRUE, then delimiting quotes not
//                                    printed
//
// RETURN: (void)
//
// PURPOSE: Write label labelList[index] to given file.  If suppressQuotes
// is TRUE and the label has delimiting end quotes, then do not print them.
//---------------------------------------------------------------------------

void WriteLabelToFile(FILE *outFile, ULONG index, LabelList *labelList,
             BOOLEAN suppressQuotes)
{
   UCHAR labelType;
   char labelStr[TOKEN_LEN];
   int strLength;
   int i;

   labelType = labelList->labels[index].labelType;
   switch(labelType) 
   {
      case STRING_LABEL:
         strcpy(labelStr, labelList->labels[index].labelValue.stringLabel);
         if (suppressQuotes) 
         { // remove delimiting DOUBLEQUOTES if there
            strLength = strlen(labelStr);
            if ((labelStr[0] == DOUBLEQUOTE) &&
                (labelStr[strLength - 1] == DOUBLEQUOTE)) 
            {
               for (i = 0; i < strLength; i++)
                  labelStr[i] = labelStr[i+1];
               labelStr[strLength - 2] = '\0';
            }
         }
         fprintf(outFile, "%s", labelStr);
         break;
      case NUMERIC_LABEL:
         fprintf(outFile, "%.*g", NUMERIC_OUTPUT_PRECISION,
                 labelList->labels[index].labelValue.numericLabel);
         break;
      default:
         break;
   }
}
