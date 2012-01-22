//---------------------------------------------------------------------------
// inccomp.c
//
// Functions for the incremental computation of the globally best substructure
//
// Subdue 5
//---------------------------------------------------------------------------

#include "subdue.h"


//---------------------------------------------------------------------------
// NAME:  AdjustMetrics
//
// INPUTS:  (Substructure *sub) - Substructure whose value is being adjusted
//          (Parameters *parameters)
//
// RETURN:  (void)
//
// PURPOSE:  Adjust the value calculations for a known sub based on the
//           new instance on an increment boundary that has been added.
//           Because examples are assumed to not cross boundary borders for
//           concept learning, this calculation is not made for setcover
//           evaluation.
//---------------------------------------------------------------------------

void AdjustMetrics(Substructure *sub, Parameters *parameters)
{
   double sizeOfPosIncrement = 0.0;
   double sizeOfNegIncrement = 0.0;
   double sizeOfSub = 0.0;
   Increment *increment;
	
   increment = GetCurrentIncrement(parameters);

   // The increment pos/neg value is stored when the increment is first read in
   // and so measures the size of the original graph
   sizeOfSub = (double) GraphSize(sub->definition);
   sizeOfPosIncrement = (double) IncrementSize(parameters,
                                       GetCurrentIncrementNum(parameters), POS);

   // Modified to only compress and measure the current increment
   sub->posIncrementValue = (double) SizeOfCompressedGraph(parameters->posGraph,
                                               sub->instances, parameters, POS);
   sub->value = sizeOfPosIncrement / (sizeOfSub + sub->posIncrementValue);
   if (parameters->negGraph != NULL)
   {
      sizeOfNegIncrement = (double) IncrementSize(parameters,
                                       GetCurrentIncrementNum(parameters), NEG);

      // Modified to only compress and measure the current increment
      sub->negIncrementValue =
                      (double) SizeOfCompressedGraph(parameters->negGraph,
                                  sub->instances, parameters, NEG);
      sub->value = (sizeOfPosIncrement + sizeOfNegIncrement) /
                   (sizeOfSub + sub->negIncrementValue +
                    sizeOfNegIncrement - sub->negIncrementValue);
   }
}


//---------------------------------------------------------------------------
// NAME: ComputeBestSubstructures
//
// INPUTS:  (Parameters *parameters)
//          (int listSize)
//
// RETURN:  (Sublist *) - substructures in ranked order
//
// PURPOSE: Traverse the top n substructures collected for each increment
//	    and compute the best n for the entire graph up to this point.
//          For each substructure that is new, check each increment to see 
//          if it contains instances, then calculate its global value and store
//          substructure on the global sublist.
//---------------------------------------------------------------------------

SubList *ComputeBestSubstructures(Parameters *parameters, int listSize)
{
   SubList *globalSubList;
   SubList *completeSubList;
   SubListNode *subIndex;
   SubListNode *incrementSubListNode;
   IncrementListNode *incNodePtr;
   Increment *increment;
   Increment *currentInc;
   double subValue;
   BOOLEAN found = FALSE;
   ULONG numPosInstances = 0;
   ULONG numNegInstances = 0;
   ULONG graphPosSize;
   ULONG graphNegSize;
   int i;
	
   completeSubList = AllocateSubList();
   incNodePtr = parameters->incrementList->head;
   currentInc = GetCurrentIncrement(parameters);
	
   // traverse each increment starting with the first increment
   while (incNodePtr != NULL)
   {
      increment = incNodePtr->increment;
      incrementSubListNode = increment->subList->head;

      // Traverse each sub in the increment
      while (incrementSubListNode != NULL) 
      {
         found = FALSE;
         subIndex = completeSubList->head;

	 // Add sub to global list if not already there
         while (subIndex != NULL) 
         {
            if (GraphMatch(subIndex->sub->definition,
                           incrementSubListNode->sub->definition,
        		   parameters->labelList, 0.0, NULL, NULL))
            {
               found = TRUE;
               break;
            }
            subIndex = subIndex->next;
         }

         if (!found)                  // add to globalSubList
         {
            subValue = ComputeValue(incNodePtr, incrementSubListNode->sub,
                          parameters->labelList, &numPosInstances,
                          &numNegInstances, &graphPosSize, &graphNegSize,
                	  parameters);

            // we just add it to the end of the list
            InsertSub(completeSubList, incrementSubListNode->sub, subValue,
                      numPosInstances, numNegInstances);
         }
         incrementSubListNode = incrementSubListNode->next;
      }
      incNodePtr = incNodePtr->next;
   }

   // create the truncated sublist
   if (listSize > 0)
   {
      globalSubList = AllocateSubList();
      subIndex = completeSubList->head;
      for (i=0; ((i<listSize) && (subIndex != NULL)); i++)
      {
         InsertSub(globalSubList, subIndex->sub, subIndex->sub->value,
                   subIndex->sub->numInstances, subIndex->sub->numNegInstances);
         subIndex = subIndex->next;
      }
      FreeSubList(completeSubList);
   }
   else 
      globalSubList = completeSubList;

   return globalSubList;
}


//---------------------------------------------------------------------------
// NAME: ComputeValue
//
// INPUTS: (IncrementListNode *incNodePtr)
//         (Substructure *sub)
//         (LabelList *labelList)
//         (ULONG *numPosInstances)
//         (ULONG *numNegInstances)
//         (ULONG *graphPosSize)
//         (ULONG *graphNegSize)
//         (Parameters *parameters)
//
// RETURN: (double)
//
// PURPOSE: Given a sub and a set of increments, compute the value of the sub.
//	    We only need to look from the current increment forward, since the
//	    sub would have to have been newly introduced in this increment.
//---------------------------------------------------------------------------

double ComputeValue(IncrementListNode *incNodePtr, Substructure *sub,
                    LabelList *labelList, ULONG *numPosInstances,
                    ULONG *numNegInstances, ULONG *graphPosSize,
                    ULONG *graphNegSize, Parameters *parameters)
{
   Increment *increment;
   IncrementListNode *incrementNode;
   SubListNode *subListNode;
   Substructure *incrementSub;
   BOOLEAN found = FALSE;
   ULONG totalPosInstances = 0;
   ULONG totalNegInstances = 0;
   ULONG totalPosEgs = 0;
   ULONG totalNegEgs = 0;
   ULONG startIncrement;
   double totalPosGraphSize = 0; // total size of uncompressed pos graph
   double totalNegGraphSize = 0; // total size of uncompressed neg graph
   double compressedPosGraphSize = 0;
   double compressedNegGraphSize = 0;
   double subValue;

   if (incNodePtr == NULL)
      printf("ComputeValue Error: incNodePtr is null\n");

   if (parameters->evalMethod != EVAL_SETCOVER)
   {
      // If the increment we are examining is not the first one, then we need
      // to collect the graph size from all previous increments in
      // which the sub did not occur.
      startIncrement = incNodePtr->increment->incrementNum;

      if (startIncrement !=
          parameters->incrementList->head->increment->incrementNum) 
      {
         incrementNode = parameters->incrementList->head;

         // traverse each increment
         while (incrementNode->increment->incrementNum < startIncrement)
         {
            increment = incrementNode->increment;
            totalPosGraphSize += increment->numPosVertices +
                                 increment->numPosEdges;
            totalNegGraphSize += increment->numNegVertices +
                                 increment->numNegEdges;
            incrementNode = incrementNode->next;
         }
         compressedPosGraphSize = totalPosGraphSize; // no compression yet
         compressedNegGraphSize = totalNegGraphSize;
      }
   }

   // Start with the increment in which the sub was found and then
   // traverse all subsequent increments looking for sub
   while (incNodePtr != NULL)
   {
      increment = incNodePtr->increment;

      if (parameters->evalMethod != EVAL_SETCOVER)
      {
         // Compute the total graph size from the size of each increment
         totalPosGraphSize += increment->numPosVertices +
                              increment->numPosEdges;
         totalNegGraphSize += increment->numNegVertices +
                              increment->numNegEdges;
      }
      subListNode = increment->subList->head;
      found = FALSE;

      // Traverse all subs in this increment and find the sub in question
      // Note that only the local best substructures are searched for a match,
      // so the number of instances may not be perfectly accurately udpated
      while (subListNode != NULL)
      {
         incrementSub = subListNode->sub;
         if (GraphMatch(incrementSub->definition, sub->definition, labelList,
                        0.0, NULL, NULL)) 
         {
            // Found it, now update the values
            if (parameters->evalMethod == EVAL_SETCOVER)
            {
               totalPosEgs += (ULONG) incrementSub->posIncrementValue;
               totalNegEgs += (ULONG) incrementSub->negIncrementValue;
            }
            else
            {
               compressedPosGraphSize += incrementSub->posIncrementValue;
               compressedNegGraphSize += incrementSub->negIncrementValue;
            }
            totalPosInstances += incrementSub->numInstances;
            totalNegInstances += incrementSub->numNegInstances;
            found = TRUE;
            break;  //if we found it we can stop
         }
         subListNode = subListNode->next;
      }

      // If this increment did not contain the substructure then we have to
      // add the full size of the increment into the compressed graph size
      if (!found && parameters->evalMethod != EVAL_SETCOVER)
      {
         compressedPosGraphSize += increment->numPosVertices +
                                   increment->numPosEdges;
         compressedNegGraphSize += increment->numNegVertices +
                                   increment->numNegEdges;
      }
      incNodePtr = incNodePtr->next;
   }

   *numPosInstances = totalPosInstances;
   *numNegInstances = totalNegInstances;
   *graphPosSize = totalPosGraphSize;
   *graphNegSize = totalNegGraphSize;

   if (parameters->evalMethod == EVAL_SETCOVER)
   {
      subValue = ((double) (totalPosEgs +
                            (parameters->numPosEgs - totalNegEgs))) /
        	 ((double) (parameters->numPosEgs + parameters->numNegEgs));
   }
   else
   {
      if (totalNegGraphSize == 0)
         subValue = totalPosGraphSize /
                    ((double) GraphSize(sub->definition) +
                     compressedPosGraphSize);
      else 
         subValue = (totalPosGraphSize + totalNegGraphSize) /
                    ((double) GraphSize(sub->definition) +
                     compressedPosGraphSize + totalNegGraphSize -
                     compressedNegGraphSize);
   }
   return(subValue);
}


//---------------------------------------------------------------------------
// NAME: InsertSub
//
// INPUTS: (SubList *masterSubList)
//         (Substructure *sub)
//         (double subValue)
//         (ULONG numPosInstances)
//         (ULONG numNegInstances)
//
// RETURN: (void)
//
// PURPOSE: We traverse all of the increments and collect the set of 
//          unique substructures to compute the overall value for the
//          graph.  This function inserts a new substructure on that
//          master list.  The list is maintained in descending order by value.
//---------------------------------------------------------------------------

void InsertSub(SubList *masterSubList, Substructure *sub, double subValue,
               ULONG numPosInstances, ULONG numNegInstances)
{
   SubListNode *subIndex;
   SubListNode *last;
   SubListNode *newSubListNode;
   Substructure *newSub;
	
   subIndex = masterSubList->head;
   newSub = CopySub(sub);
   newSub->numInstances = numPosInstances;
   newSub->numNegInstances = numNegInstances;
   newSub->value = subValue;
   newSubListNode = AllocateSubListNode(newSub);
   last = NULL;
	
   // insert in descending order
   if (subIndex == NULL)
      masterSubList->head = newSubListNode;
   else
   {
      while (subIndex != NULL)
      {
         // keep going until we find the place to insert
         if (subIndex->sub->value > newSub->value)
         {
            last = subIndex;
            subIndex = subIndex->next;
         }
         else // insert
         {
            newSubListNode->next = subIndex;
            if (last != NULL)
               last->next = newSubListNode;
            else  //insert at beginning of list
               masterSubList->head = newSubListNode;
            return;
         }
      }
      // the case where we insert at the end of the list
      last->next = newSubListNode;
   }
}
