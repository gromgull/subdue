//---------------------------------------------------------------------------
// incutil.c
//
// Set of functions to deal with adding, retrieving and manipulating data 
// increments.  It is important to note that the increment list does not store
// the actual data.  The graph is stored as it always was, in the posGraph 
// list.  The increment structure provides an index into that list, along with
// some other necessary parameters.
//
// SUBDUE 5
//---------------------------------------------------------------------------

#include "subdue.h"


//---------------------------------------------------------------------------
// NAME: IncrementSize
//
// INPUTS: (Parameters *parameters) - system parameters
//         (ULONG incrementNum) - current increment
//         (ULONG graphType) - POS or NEG
//
// RETURN: (ULONG) - size of increment
//
// PURPOSE: Return size of graph as vertices plus edges.
//---------------------------------------------------------------------------

ULONG IncrementSize(Parameters *parameters, ULONG incrementNum, ULONG graphType)
{
   Increment *increment = GetIncrement(incrementNum, parameters);

   if (graphType == POS)
      return(increment->numPosVertices + increment->numPosEdges);
   else 
      return(increment->numNegVertices + increment->numNegEdges);
}


//---------------------------------------------------------------------------
// NAME: IncrementNumExamples
//
// INPUTS: (Parameters *parameters) - system parameters
//         (ULONG incrementNum) - current increment
//         (ULONG graphType) - POS or NEG
//
// RETURN: (ULONG) - number of edges
//
// PURPOSE: Return number of positive or negative examples in increment.
//---------------------------------------------------------------------------

ULONG IncrementNumExamples(Parameters *parameters, ULONG incrementNum,
                           ULONG graphType)
{
   Increment *increment = GetIncrement(incrementNum, parameters);

   if (graphType == POS)
      return((ULONG) increment->numPosEgs);
   else 
      return((ULONG) increment->numNegEgs);
}


//---------------------------------------------------------------------------
// NAME:  WriteResultsToFile
//
// INPUTS: (SubList *subList) - pointer to list of substructures
//         (FILE *subsFile) - file holding substructures
//         (Increment *increment) - current increment
//         (Parameters *parameters) - system parameters
//
// RETURN: (void)
//
// PURPOSE: Write results from increment to file.
//---------------------------------------------------------------------------

void WriteResultsToFile(SubList *subList, FILE *subsFile,
                        Increment *increment, Parameters *parameters)
{
   SubListNode *subListNode = NULL;
   ULONG subSize;
   ULONG incSize;

   incSize = increment->numPosVertices + increment->numPosEdges +
             increment->numNegVertices + increment->numNegEdges;
   if (subList != NULL)
   {
      subListNode = subList->head;
      if(subListNode != NULL)
      {
         subSize = subListNode->sub->definition->numVertices +
                   subListNode->sub->definition->numEdges;
         WriteGraphToFile(subsFile, subListNode->sub->definition,
                          parameters->labelList, 0, 0, 
                          subListNode->sub->definition->numVertices, TRUE);
      }
   }
}


//-------------------------------------------------------------------------
// NAME:  GetOutputFileName
//
// INPUTS:  (char *suffix) - output file name suffix (before "_")
//          (ULONG index) - output file name index (between "_" and ".txt");
//
// RETURN:  (char *) - name of output file
//
// PURPOSE: Generate name of output file for current increment.
//-------------------------------------------------------------------------

char *GetOutputFileName(char *suffix, ULONG index)
{
   char *fileName;
   char str[20];

   fileName = malloc(sizeof(char) * 50);

   sprintf(str, "_%d", (int) index);
   strcpy(fileName, suffix);
   strcat(fileName, str);
   strcat(fileName, ".txt");

   return fileName;
}


//---------------------------------------------------------------------------
// NAME:  CopySub
//
// INPUTS:  (Substructure *sub) - input substructure
//
// RETURN:  (Substructure *sub) - copy of input substructure
//
// PURPOSE:  Make new copy of existing substructure.
//---------------------------------------------------------------------------

Substructure *CopySub(Substructure *sub)
{
   Substructure *newSub;

   newSub = (Substructure *) malloc(sizeof(Substructure));

   newSub->definition = CopyGraph(sub->definition);
   newSub->posIncrementValue = sub->posIncrementValue;
   newSub->negIncrementValue = sub->negIncrementValue;
   newSub->value = sub->value;
   newSub->numInstances = sub->numInstances;
   newSub->instances = NULL;
   newSub->numNegInstances = sub->numNegInstances;
   newSub->negInstances = NULL;
   newSub->recursive = sub->recursive;
   newSub->recursiveEdgeLabel = sub->recursiveEdgeLabel;

   return(newSub);
}


//---------------------------------------------------------------------------
// NAME:  StoreSubs
//
// INPUTS:  (SubList *subList) - pointer to list of substructures
//          (Parameters *parameters) - system parameters
//
// RETURN:  void
//
// PURPOSE: Store the local subs discovered in this increment.
//          There is no need to store the actual instances.  We never need them
//          again, only the count.  To deal with overlapping vertices in 
//          boundary instances, we need to keep the instances for the current 
//          and previous increment.  We can get rid of instances for all other 
//          previous increments.
//---------------------------------------------------------------------------

void StoreSubs(SubList *subList, Parameters *parameters)
{
   SubListNode *subListNode;
   ULONG currentIncrement = GetCurrentIncrementNum(parameters);
   Increment *increment = GetIncrement(currentIncrement, parameters);

   increment->subList = subList;

   AddVertexTrees(subList, parameters);

   subListNode = increment->subList->head;
   while(subListNode != NULL)
   {
      FreeInstanceList(subListNode->sub->instances);
      subListNode->sub->instances = NULL;
      subListNode = subListNode->next;
   }
}


//---------------------------------------------------------------------------
// NAME:  AddVertexTrees
//
// INPUTS:  (SubList *subList) - pointer to list of substructures
//          (Parameters *parameters) - system parameters
//
// RETURN:  void
//
// PURPOSE:  For the list of subs in this increment, collect all of the vertex
//           indices from the instance lists and store them in an avl tree.  
//           Store one avl tree for each sub in corresponding order
//---------------------------------------------------------------------------

void AddVertexTrees(SubList *subList, Parameters *parameters)
{
   SubListNode *subListNode;
   Substructure *sub;
   InstanceListNode *instanceListNode;
   Instance *instance;
   struct avl_table* avlTable;

   subListNode = subList->head;
   while (subListNode != NULL)
   {
      sub = subListNode->sub;
      avlTable = GetSubTree(sub, parameters);
      if (avlTable == NULL)
      {
         avlTable = avl_create((avl_comparison_func *)compare_ints, NULL, NULL);
         AddInstanceVertexList(sub, avlTable, parameters);
      }
      instanceListNode = sub->instances->head;
      while (instanceListNode != NULL)
      {
         instance = instanceListNode->instance;
         AddInstanceToTree(avlTable, instance);
         instanceListNode = instanceListNode->next;
      }
      subListNode = subListNode->next;
   }
}


//---------------------------------------------------------------------------
// NAME:  AddInstanceToTree
//
// INPUTS:  (struct avl_table *avlTable) - tree being traversed
//          (Instance *instance) - substructure instance
//
// RETURN:  void
//
// PURPOSE:  Add a substructure instance to the avl tree.
//---------------------------------------------------------------------------

void AddInstanceToTree(struct avl_table *avlTable, Instance *instance)
{
   ULONG *vertices;
   ULONG *j;
   ULONG i;

   vertices = instance->vertices;
   for (i=0;i<instance->numVertices;i++)
   {
      j = (ULONG*) malloc(sizeof(ULONG));
      *j = vertices[i];
      avl_insert(avlTable, j);
   }
}


//---------------------------------------------------------------------------
// NAME:  GetSubTree
//
// INPUTS:  (Substructure *sub) - current substructure
//          (Parameters *parameters) - system parameters
//
// RETURN:  (struct avl_table *) - pointer to avl tree
//
// PURPOSE:  Return the avl tree corersponding to the input substructure.
//---------------------------------------------------------------------------

struct avl_table *GetSubTree(Substructure *sub, Parameters *parameters)
{
   AvlTreeList *avlTreeList;
   AvlTableNode *avlTableNode;
   BOOLEAN found = FALSE;

   avlTreeList = parameters->vertexList->avlTreeList;
   avlTableNode = avlTreeList->head;

   while ((avlTableNode != NULL) && !found)
   {
      if (GraphMatch(sub->definition, avlTableNode->sub->definition,
                     parameters->labelList, 0.0, NULL, NULL))
         found = TRUE;
      else 
         avlTableNode = avlTableNode->next;
   }

   if (found)
      return avlTableNode->vertexTree;
   else 
      return NULL;
}


//---------------------------------------------------------------------------
// NAME:  AddInstanceVertexList
//
// INPUTS:  (Substructure *sub) - current substructure
//          (struct avl_table *avlTable) - pointer to avl tree
//          (Parameters *parameters) - system parameters
//
// RETURN:  void
//
// PURPOSE:  Add the new avl tree to the head of the list of avl trees.
//---------------------------------------------------------------------------

void AddInstanceVertexList(Substructure *sub, struct avl_table *avlTable,
                           Parameters *parameters)
{
   AvlTableNode *avlTableNode;
   AvlTreeList *avlTreeList;

   avlTreeList = parameters->vertexList->avlTreeList;

   avlTableNode = malloc(sizeof(AvlTableNode));
   avlTableNode->vertexTree = avlTable;
   avlTableNode->sub = sub;
   avlTableNode->next = avlTreeList->head;
   avlTreeList->head = avlTableNode;
}


//---------------------------------------------------------------------------
// NAME:  compare_ints
//
// INPUTS:  (const void *pa) - first value
//          (const void *pb) - second value
//
// RETURN:  int - result of comparison (-1: >; 1: >; 0: =)
//
// PURPOSE:  Return the result of comparing two input values.
//---------------------------------------------------------------------------

int compare_ints(const void *pa, const void *pb)
{
   const int *a = pa;
   const int *b = pb;

   if (*a < *b)
      return -1;
   else if (*a > *b)
      return 1;
   else 
      return 0;
}


//---------------------------------------------------------------------------
// NAME: AddNewIncrement
//
// INPUTS: (ULONG startPosVertexIndex) - first positive vertex in new increment
//         (ULONG startPosEdgeIndex) - first positive edge in new increment
//         (ULONG startNegVertexIndex) - first negative vertex in new increment
//         (ULONG startNegEdgeIndex) - index of negative edge in new increment
//	   (ULONG numPosVertices) - number of positive vertices in new increment
//	   (ULONG numPosEdges) - number of positive edges in new increment
//	   (ULONG numNegVertices) - number of negative vertices in new increment
//	   (ULONG numNegEdges) - number of negative edges in new increment
//	   (Parameters *parameters) - pointer to global parameters
//
// RETURN: void
//
// PURPOSE: Adds a new increment, which consists of an index into the array of
//	    vertices, the number of the current increment, and the list of 
//          associated subs (to be added later).
//---------------------------------------------------------------------------

void AddNewIncrement(ULONG startPosVertexIndex, ULONG startPosEdgeIndex,
                     ULONG startNegVertexIndex, ULONG startNegEdgeIndex,
                     ULONG numPosVertices, ULONG numPosEdges,
                     ULONG numNegVertices, ULONG numNegEdges, 
                     Parameters *parameters)
{
   IncrementListNode *incNodePtr = parameters->incrementList->head;
   IncrementListNode *currentIncrementListNode =
      malloc(sizeof(IncrementListNode));
   Increment *increment = malloc(sizeof(Increment));
   increment->subList = AllocateSubList();

   currentIncrementListNode->increment = increment;
   if (incNodePtr == NULL) // Empty list, this is the first increment
   {
      increment->incrementNum = 1;
      parameters->incrementList->head = currentIncrementListNode;
   }
   else
   {
      while(incNodePtr->next != NULL)
         incNodePtr = incNodePtr->next;
      increment->incrementNum = incNodePtr->increment->incrementNum + 1;
      incNodePtr->next = currentIncrementListNode;
   }
   increment->startPosVertexIndex = startPosVertexIndex;
   increment->startPosEdgeIndex = startPosEdgeIndex;
   increment->startNegVertexIndex = startNegVertexIndex;
   increment->startNegEdgeIndex = startNegEdgeIndex;
   increment->numPosVertices = numPosVertices;
   increment->numPosEdges = numPosEdges;
   increment->numNegVertices = numNegVertices;
   increment->numNegEdges = numNegEdges;
   currentIncrementListNode->next = NULL;
}


//---------------------------------------------------------------------------
// NAME GetIncrement
//
// INPUTS:  (ULONG incrementNum) - current increment
//          (Parameters *parameters) - system parameters
//
// RETURN:  (Increment *) - increment structure
//
// PURPOSE: returns the actual increment structure, with its parameters
//---------------------------------------------------------------------------

Increment *GetIncrement(ULONG incrementNum, Parameters *parameters)
{
   IncrementListNode *listNode = GetIncrementListNode(incrementNum, parameters);
   if (listNode == NULL)
      return NULL;

   return listNode->increment;
}


//---------------------------------------------------------------------------
// NAME: GetCurrentIncrement
//
// INPUTS:  (Parameters *) - system parameters
//
// RETURN:  (Increment *) - current increment structure
//
// PURPOSE: returns a pointer to the current increment structure
//---------------------------------------------------------------------------

Increment *GetCurrentIncrement(Parameters *parameters)
{
   ULONG currentIncrementNum = GetCurrentIncrementNum(parameters);
   Increment *increment = GetIncrement(currentIncrementNum, parameters);
   if (currentIncrementNum == 0)
      return NULL;
   return increment;
}


//---------------------------------------------------------------------------
// NAME: GetCurrentIncrementNum
//
// INPUTS:  (Parameters *) - system parameters
//
// RETURN:  (ULONG) - current increment number
//
// PURPOSE: returns the sequential number of the current increment
//---------------------------------------------------------------------------

ULONG GetCurrentIncrementNum(Parameters *parameters)
{
   IncrementListNode *incNodePtr = parameters->incrementList->head;

   if (incNodePtr == NULL) //empty list, this is the first increment
      return 0;
   while (incNodePtr->next != NULL)
      incNodePtr = incNodePtr->next;

   return incNodePtr->increment->incrementNum;	
}


//---------------------------------------------------------------------------
// NAME: SetIncrementNumExamples
//
// INPUTS: (Parameters *parameters) - system parameters
//
// RETURN:  void
//
// PURPOSE:  Set the description lengths of an increment.  This is only 
//           called for the current increment, so we assume that parameter
//---------------------------------------------------------------------------

void SetIncrementNumExamples(Parameters *parameters)
{
   Increment *increment = GetCurrentIncrement(parameters);
   ULONG i;
   ULONG numEgs;
   ULONG start;

   start = increment->startPosVertexIndex;
   numEgs = parameters->numPosEgs;
   for (i=0; i<numEgs; i++)
      if (parameters->posEgsVertexIndices[i] >= start)  // Examples in increment
         break;
   increment->numPosEgs = (double) (numEgs - i);

   start = increment->startNegVertexIndex;
   numEgs = parameters->numNegEgs;
   for (i=0; i<numEgs; i++)
      if (parameters->negEgsVertexIndices[i] >= start)  // Examples in increment
         break;
   increment->numNegEgs = (double) (numEgs - i);
}


//---------------------------------------------------------------------------
// NAME: GetStartVertexIndex
//
// INPUTS:  (ULONG incrementNum) - current increment
//          (Parameters *parameters) - system parameters
//          (ULONG graphType) - POS or NEG
//
// RETURN:  ULONG - starting vertex number for corresponding increment
//
// PURPOSE: Return the index of the starting positive vertex for the increment.
//---------------------------------------------------------------------------

ULONG GetStartVertexIndex(ULONG incrementNum, Parameters *parameters,
                          ULONG graphType)
{
   IncrementListNode *incNodePtr = parameters->incrementList->head;

   if (incNodePtr == NULL) // Empty list, this is the first increment
   {
      printf("Error GetStartVertexIndex: increment list empty\n");
      exit(1);
   }
   while (incNodePtr->next != NULL)
      incNodePtr = incNodePtr->next;

   if (graphType == POS)
      return incNodePtr->increment->startPosVertexIndex;
   else 
      return incNodePtr->increment->startNegVertexIndex;
}


//---------------------------------------------------------------------------
// NAME: GetIncrementListNode
//
// INPUTS:  (ULONG incrementNum) - current increment
//          (Parameters *parameters) - system parameters
//
// RETURN:  (IncrementListNode *) - corresponding increment nocde
//
// PURPOSE: Return a node from the increment list, corresponding to a particular
//          increment number.  Only used internal to this module.
//---------------------------------------------------------------------------

IncrementListNode *GetIncrementListNode(ULONG incrementNum,
                                        Parameters *parameters)
{
   IncrementListNode *incNodePtr = parameters->incrementList->head;

   if (incNodePtr == NULL) //empty list, this is the first increment
      return NULL;
   while (incNodePtr->next != NULL)
      incNodePtr = incNodePtr->next;
   return incNodePtr;	
}


//---------------------------------------------------------------------------
// NAME: PrintStoredSubList
//
// INPUTS: (SubList *subList) - list of substructures to print
//         (Parameters *parameters) - system parameters
//
// RETURN:  void
//
// PURPOSE: Print given list of substructures.
//---------------------------------------------------------------------------

void PrintStoredSubList(SubList *subList, Parameters *parameters)
{
   ULONG counter = 1;
   ULONG num = parameters->numBestSubs;
   SubListNode *subListNode = NULL;

   if (subList != NULL)
   {
      subListNode = subList->head;
      while ((subListNode != NULL) && (counter <= num))
      {
         printf("(%lu) ", counter);
         counter++;
         PrintStoredSub(subListNode->sub, parameters);
         printf("\n");
         subListNode = subListNode->next;
      }
   }
}


//---------------------------------------------------------------------------
// NAME: PrintStoredSub
//
// INPUTS: (Substructure *sub) - substructure to print
//         (Parameters *parameters) - parameters
//
// RETURN: void
//
// PURPOSE: Print given substructure's value, number of instances,
//          definition, and possibly the instances.
//---------------------------------------------------------------------------

void PrintStoredSub (Substructure *sub, Parameters *parameters)
{
   if (sub != NULL)
   {
      printf("Substructure: value = %.*g, ", NUMERIC_OUTPUT_PRECISION,
 	     sub->value);
      printf("pos instances = %lu, neg instances = %lu\n",
 	     sub->numInstances, sub->numNegInstances);
      if (sub->definition != NULL)
         PrintGraph(sub->definition, parameters->labelList);  
   }
}
