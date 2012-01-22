//---------------------------------------------------------------------------
// incboundary.c
//
// Functions for the discovering substructure instances that cross a
// sequential increment boundary.
//
// Subdue 5
//---------------------------------------------------------------------------

#include "subdue.h"


//---------------------------------------------------------------------------
// NAME: EvaluateBoundaryInstances
//
// INPUT: (SubList *bestSubList) - globally best subs known to this point
//        (Parameters *parameters) - system parameters
//
// RETURN: (InstanceList *) - a list of all instances that span the increment 
//                            boundary
//
// PURPOSE: This function is called after each local increment is processed.
//          It finds any instances of the top-n substructures that span from
//          the current increment back to previous increments.
//---------------------------------------------------------------------------

InstanceList *EvaluateBoundaryInstances(SubList *bestSubList,
                                        Parameters *parameters)
{
   InstanceListNode *instanceListNode;
   // final set of instances that match up with subs
   InstanceList *finalInstanceList;
   Instance *instance;
   SubListNode *subListNode;
   Substructure *bestSub;
   BOOLEAN empty;
   RefInstanceList *refInstanceList;
   ReferenceGraph *refGraph = NULL;
   RefInstanceListNode *refInstanceListNode = NULL;

   empty = FindInitialBoundaryInstances(bestSubList, parameters->posGraph,
              parameters->labelList,
              GetCurrentIncrement(parameters)->startPosVertexIndex, parameters);
   if (empty)
      return NULL;

   // holds the final set of boundary instances
   finalInstanceList = AllocateInstanceList();

   // For each top-n sub, create a list of refGraphs that correspond to each 
   // candidate instance
   subListNode = bestSubList->head;
   while (subListNode != NULL)
   {
      // List of RefInstanceListNodes for a particular top-n sub, one for each
      // initial seed instance
      refInstanceList = AllocateRefInstanceList();
      bestSub = subListNode->sub;
      if (parameters->outputLevel > 2)
      {
         printf("Processing Instances for Top-n Sub:\n");
         PrintStoredSub(bestSub, parameters);
      }
      if (bestSub->instances != NULL)
      {
         instanceListNode = bestSub->instances->head;
         while (instanceListNode != NULL)
         {
            // The RefInstanceListNode holds a reference graph and the list of 
            // instances that use it.  We create one RefInstanceListNode for
            // each initial seed instance.
            refInstanceListNode = AllocateRefInstanceListNode();
            instance = instanceListNode->instance;
            InstanceListInsert(instance, refInstanceListNode->instanceList,
                               FALSE);
            refGraph =
               InstanceToRefGraph(instance, parameters->posGraph, parameters);
            refInstanceListNode->refGraph = refGraph;
            refInstanceListNode->firstPass = TRUE;
            refInstanceListNode->doExtend = TRUE;

            // Insert at head of list
            refInstanceListNode->next = refInstanceList->head;
            refInstanceList->head = refInstanceListNode;

            instanceListNode = instanceListNode->next;
         }
      }
      FreeInstanceList(bestSub->instances);
      bestSub->instances = NULL;
      bestSub->numInstances = 0;

      // For each top-n sub, store the set of refGraphs

      ProcessInstancesForSub(bestSub, refInstanceList, finalInstanceList, parameters);
      free(refInstanceList);
      subListNode = subListNode->next;
   }

   return finalInstanceList;
}

	
//-----------------------------------------------------------------------------
// NAME:  ProcessInstancesForSub
//
// INPUT:  (Substructure *bestSub)-  a sub from the top-n list
//         (RefInstanceList *refInstanceList) - initial list of seed instances
//                                              for the sub
//         (InstanceList *finalInstanceList)  - final set of complete instances
//                                              for the sub
//         (Parameters *parameters) - system parms
//
// RETURN: (void)
//
// PURPOSE: Grow the seed instances into instances of a top-n substructure.
//-----------------------------------------------------------------------------

void ProcessInstancesForSub(Substructure *bestSub,
                            RefInstanceList *refInstanceList, 
                            InstanceList *finalInstanceList,
                            Parameters *parameters)
{
   RefInstanceListNode *refInstanceListNode = NULL;
   ReferenceGraph *refGraph = NULL;
   Instance *gInstance;
   InstanceList *bestSubInstanceList;
   InstanceListNode *instanceListNode;
   InstanceList *remainingInstances = NULL;
   Instance *instance;
   Substructure *newSub;
   BOOLEAN listEmpty;
   BOOLEAN instanceAdded;
   BOOLEAN empty;
   BOOLEAN doPrune;
   ULONG index = 1;
   ULONG index2;
   ULONG kept;
   ULONG maxVertexCount;

   // Now we go through the process of trying to grow the seed instances into
   // instances of the top-n substructures.
   // Any instance returned on the instance list of a top-n sub must be a
   // subset of a top-n sub.  We now must check all of the instances and see if
   // they are an exact match to the top-n sub.
   refInstanceListNode = refInstanceList->head;

   // Each refInstanceList has multiple refInstanceListNodes, each containing
   // an instance list and a refGraph
   while (refInstanceListNode != NULL)
   {
      empty = FALSE;
      while (!empty)
      {
         bestSubInstanceList = refInstanceListNode->instanceList;
         refGraph = refInstanceListNode->refGraph;
         remainingInstances = NULL;
         if (bestSubInstanceList != NULL)
         {
            instanceListNode = bestSubInstanceList->head;
         }
         else
            instanceListNode = NULL;

         listEmpty = TRUE;
         remainingInstances = NULL;

         // check every instance to see if it is an exact match to the sub
         index2 = 0;
         kept = 0;
         doPrune = FALSE;
         maxVertexCount = 0;

         while (instanceListNode != NULL)
         {
            index2++;
            instance = instanceListNode->instance;

            // This match check is a little convoluted at the moment.
            // It could be simplified by converting the ref instance directly
            // to a graph.

            // Create a copy of the instance and reference the full graph.
            // We have to do that to turn it into a sub that we can use for a 
            // match check.
            gInstance = CreateGraphRefInstance(instance, refGraph);
            newSub = CreateSubFromInstance(gInstance, parameters->posGraph);
            instanceAdded = FALSE;
            if (parameters->outputLevel > 2)
            {
               printf("\ninstance num: %lu\n", index2);
       	       PrintInstance(gInstance, 0, parameters->posGraph, parameters->labelList);
            }

            // Check to see if the instance is an exact match to the bestSub
            if (GraphMatch(newSub->definition, bestSub->definition,
	        parameters->labelList, 0.0, NULL, NULL))
            {
               // Update the increment data structure to reflect new instance.
	       // The instance will not be added if there are vertices that
	       // overlap with previously-added instances, from this or
	       // previous increments.
               instanceAdded =
	          AddInstanceToIncrement(newSub, gInstance, parameters);

               // keeping a final list of instances is technically unnecessary
               if (instanceAdded)
               {
                  InstanceListInsert(gInstance, finalInstanceList, FALSE);
                  doPrune = TRUE;
               }
            }

            // If not then keep the instance around for further extension
            else
            {
               if (listEmpty)
               {
                  remainingInstances = AllocateInstanceList();
                  listEmpty = FALSE;
               }
               kept++;
               InstanceListInsert(instance, remainingInstances, TRUE);
               FreeInstance(gInstance);
               if (instance->numVertices > maxVertexCount)
                  maxVertexCount = instance->numVertices;
            }
            FreeSub(newSub);
            instanceListNode = instanceListNode->next;
         }

         FreeInstanceList(refInstanceListNode->instanceList);
         refInstanceListNode->instanceList = NULL;
         if (parameters->outputLevel > 2)
         {
            printf("Examined %lu candidate instances, kept %lu for extension\n",
                   index2, kept);
            printf("maxVertexCount = %lu\n", maxVertexCount);
         }

         // If we found an instance and added it to the increment, then we can
         // prune any remaining instances that overlap, since we never
         // add duplicate instances
         if (doPrune && (remainingInstances != NULL))
            remainingInstances =
               PruneCandidateList(bestSub, refInstanceListNode->refGraph,
                                  remainingInstances, parameters);

         // Store the remaining instances for further extension.
         // The list may be empty.
         if (remainingInstances != NULL)
         {
            if (parameters->outputLevel > 2)
               printf("Extend next set: %lu\n", index);
            refInstanceListNode->instanceList = remainingInstances;

            // At this point we have removed any instance that was an exact
            // match to a top-n sub.

            // Extend the ref graphs for the next iteration.  
            CreateExtendedGraphs(refInstanceListNode, bestSub,
                                 parameters->posGraph, parameters);

            // For each instance on the list, returns the next extension of
            // the instance that is either a subset or equal to an existing
            // top-n sub.  If no instance could be extended then we are done.
            empty = ExtendBoundaryInstances(bestSub, refInstanceListNode,
                                            parameters);
         }
         else
         {
            empty = TRUE;
            //printf("Empty: %lu\n", index);
         }
         index++;
      }
      RemoveFromList(refInstanceList, &refInstanceListNode);
   }
}


//------------------------------------------------------------------------------
// NAME:  PruneCandidateList
//
// INPUTS:  (Substructure *bestSub)
//          (ReferenceGraph *refGraph)
//          (InstanceList *instanceList)
//          (Parameters *parameters)
//
// RETURN:  (InstanceList *)
//
// PURPOSE:  After an instance has been found and added to the increment
//           instance list, we can remove any other candidate instances that
//           have overlapping vertices.
//------------------------------------------------------------------------------

InstanceList *PruneCandidateList(Substructure *bestSub,
                                 ReferenceGraph *refGraph, 
                                 InstanceList *instanceList, 
                                 Parameters *parameters)
{
   InstanceListNode *instanceListNode;
   InstanceList *keepList = AllocateInstanceList();
   BOOLEAN overlap;
   Instance *gInstance;
	
   instanceListNode = instanceList->head;

   while (instanceListNode != NULL)
   {
      gInstance = CreateGraphRefInstance(instanceListNode->instance, refGraph);
      overlap = CheckInstanceForOverlap(gInstance, bestSub, parameters);
      FreeInstance(gInstance);

      if (!overlap)
         InstanceListInsert(instanceListNode->instance, keepList, FALSE);
      instanceListNode = instanceListNode->next;
   }
   FreeInstanceList(instanceList);
   return keepList;
}

//------------------------------------------------------------------------------
// NAME:: CreateExtendedGraphs
//
// INPUT: (RefInstanceListNode *refInstanceListNode) - array of refInstanceLists
//        (Substructure *bestSubList) - list of top-n subs
//        (Graph *graph) - the full graph
//        (Parameters *parameters) - system parameters
//
// RETURN:  void
//
// PURPOSE:  Calls functions to extend each ref graph by one edge and possibly
//           one vertex in all possible ways.  The extension is only performed
//           if the instance evaluation has completely exhausted the current
//           extension, recognized by the doExtend flag.
//------------------------------------------------------------------------------

void CreateExtendedGraphs(RefInstanceListNode *refInstanceListNode,
                          Substructure *bestSub, Graph *graph, 
                          Parameters *parameters)
{
   ReferenceGraph *newRefGraph;
	
   // Extend the graphs
   if (refInstanceListNode != NULL)
   {
      if (refInstanceListNode->doExtend)
      {
         newRefGraph = ExtendRefGraph(refInstanceListNode->refGraph, bestSub, graph, parameters);
         FreeReferenceGraph(refInstanceListNode->refGraph);
         refInstanceListNode->refGraph = newRefGraph;
         refInstanceListNode->firstPass = TRUE;
         refInstanceListNode->doExtend = FALSE;
      }
   }
}


//---------------------------------------------------------------------------
// NAME: FindInitialBoundaryInstances
//
// INPUT:  (SubList *bestSubList) - top n subs
//         (Graph *graph) - the accumulated graph
//         (LabelList *labelList) - accumulated collection of labels
//         (ULONG startVertexIndex) - index of the first vertex
//                                    in the current increment
//         (Parameters *parameters) - system parameters
//
// RETURN:  BOOLEAN
//
// PURPOSE:  Called once by EvaluateBoundaryInstances.  Look at each vertex
//           that has a boundary edge and see if it could be a part of a top-n
//           substructure instance.  Add the instance to the instance list of
//           each top-n sub for which we find a match.  This has the effect of
//           creating a set of 2-vertex instances that we use as a starting
//           point for extension.
//---------------------------------------------------------------------------

BOOLEAN FindInitialBoundaryInstances(SubList *bestSubList, Graph *graph,
                                     LabelList *labelList, 
                                     ULONG startVertexIndex, 
                                     Parameters *parameters)
{
   Vertex *vertices;
   Vertex v;
   Edge *edges;
   Edge e;
   ULONG i, j;
   Instance *instance;
   Instance *extInstance;
   Instance *newInstance;
   Substructure *sub;
   BOOLEAN foundMatch;
   BOOLEAN empty = TRUE;
   SubListNode *subListNode;
   Substructure *bestSub;
			
	
   vertices = graph->vertices;
   edges = graph->edges;
	
   // Look at each vertex in this increment
   for (i=startVertexIndex;i<graph->numVertices;i++)
   {
      v = vertices[i];
      // Look at each edge for this vertex
      for (j=0;j<v.numEdges;j++)
      {
         e = edges[v.edges[j]];

         // Proceed only if the edge spans the increment and both vertices
         // could be part of a top-n sub
         if (e.spansIncrement &&
             (VertexInSubList(bestSubList, &vertices[e.vertex1])) &&
             (VertexInSubList(bestSubList, &vertices[e.vertex2])))
         {
            // refCount set to 0
            instance = AllocateInstance (1, 0);
            instance->vertices[0] = i;
            instance->minMatchCost = 0.0;
            extInstance =
               CreateExtendedInstance(instance, i, v.edges[j], graph);
            FreeInstance(instance);
        				
            // Run through each top-n sub and add the instance to the list
            // anywhere we find a sgiso match.
            // To use the existing code we have to turn it into a sub.
            sub = CreateSubFromInstance(extInstance, parameters->posGraph);
            subListNode = bestSubList->head;
            while(subListNode != NULL)
            {
               bestSub = subListNode->sub;
               foundMatch = CheckForSubgraph(sub->definition,
                                             bestSub->definition, parameters);

               // If we found a sgiso match then make sure the vertices do not
               // overlap with another instance already on the sub's list
               // and make sure the vertices have not already been used for
               // another instance in a previous increment.
               if (foundMatch &&
                !CheckForSeedInstanceOverlap(extInstance, bestSub->instances) &&
                !CheckInstanceForOverlap(extInstance, bestSub, parameters))
               {
                  if (bestSub->instances == NULL)
                     bestSub->instances = AllocateInstanceList();
                  newInstance = CopyInstance(extInstance);

                  // This insertion increments the instance's refCount 
                  // If the instance is already on the list it will be freed
                  InstanceListInsert(newInstance, bestSub->instances, TRUE);
                  empty = FALSE;
               }
               subListNode = subListNode->next;
            }
            FreeSub(sub);
            FreeInstance(extInstance);
         }
      }
   }
   return empty;
}

//---------------------------------------------------------------------------
// NAME:  CheckForSeeInstanceOverlap
//
// INPUT:  (Instance *candidateInstance)
//         (InstanceList *instanceList)
//
// RETURN:  BOOLEAN
//
// PURPOSE: Called by FindInitialBoundaryInstances.  Make sure we do not add
//          seed instances with overlapping vertices.
//---------------------------------------------------------------------------

BOOLEAN CheckForSeedInstanceOverlap(Instance *candidateInstance,
                                    InstanceList *instanceList)
{
   InstanceListNode *instanceListNode;
   Instance *subInstance;
   ULONG i, j;	

   if (instanceList != NULL)
   {
      for (i=0;i<candidateInstance->numVertices;i++)
      {
         instanceListNode = instanceList->head;
         while (instanceListNode != NULL)
         {
            subInstance = instanceListNode->instance;
            for (j=0;j<subInstance->numVertices;j++)
            {
               if (candidateInstance->vertices[i] == subInstance->vertices[j]) 
                  return TRUE;
            }
            instanceListNode = instanceListNode->next;
         }
      }
   }
   return FALSE;
}

//---------------------------------------------------------------------------
// NAME: ExtendBoundaryInstances
//
// INPUTS:  (Substructure *bestSubList) - set of top n subs
//          (RefInstanceListNode *refInstanceListSet) - array of
//             refInstanceLists, one for each top-n sub
//          (Parameters *parameters) - system parameters
//
// RETURN:  BOOLEAN
//
// PURPOSE:  For each top-n sub we have created a reference graph and a
//           corresponding set of instances that are subsets of the top-n sub,
//           which we are attempting to grow into an instance of a top-n sub.
//           This function moves through the list of top-n subs and tries to
//           grow the instances associated with each sub with respect to the
//           reference graph.  Returns true if no extensions were possible,
//           signifying that we are done.
//---------------------------------------------------------------------------

BOOLEAN ExtendBoundaryInstances(Substructure *bestSub,
                                RefInstanceListNode *refInstanceListNode, 
                                Parameters *parameters)
{
   Instance *bestInstance;
   InstanceList *bestSubInstanceList;
   InstanceList *extendedInstances = NULL;
   InstanceList *candidateInstances = NULL;
   InstanceListNode *instanceListNode;
   BOOLEAN empty = FALSE;
   BOOLEAN firstPass;
   BOOLEAN foundInstances;
   ReferenceGraph *refGraph;
   Graph *fullGraph;
	
   fullGraph = parameters->posGraph;
   extendedInstances = NULL;
   if (refInstanceListNode != NULL)
   {
      firstPass = refInstanceListNode->firstPass;
      candidateInstances = AllocateInstanceList();
      bestSubInstanceList = refInstanceListNode->instanceList;
      refGraph = refInstanceListNode->refGraph;
      foundInstances = FALSE;
      if (bestSubInstanceList != NULL)
      {
         instanceListNode = bestSubInstanceList->head;
         // process each instance on the candidate list
         while (instanceListNode != NULL)
         {
            bestInstance = instanceListNode->instance;
            // try to extend the instance into a new set of instances, with
            // respect to the reference graph
            extendedInstances = ExtendConstrainedInstance(bestInstance, bestSub,
                                refGraph, fullGraph, parameters);
            // if we found extended instances then process them
            if (extendedInstances != NULL)
            {
               foundInstances = TRUE;
               // check to see which of the extended instances are actually
               // still viable in terms of their ability to grow into the top-n
               // sub we are processing
               ProcessExtendedInstances(extendedInstances, candidateInstances,
                                        bestSub, refGraph, fullGraph, 
                                        parameters);
               // at this point the candidate instances are on the
               // candidateInstances list
               FreeInstanceList(extendedInstances);
            }

            // Since we only extend the ref graph after we run out of
            // extendable instances, we need to keep failed instances around
            // for another iteration.  It is possible this instance is good but
            // we just could not extend it until the ref graph was extended.
            // FirstPass is set to true if this is the first attempt to extend
            // instances after the ref graph has been extended.
           else if (!firstPass &&
	            !MemberOfInstanceList(bestInstance, candidateInstances))
              InstanceListInsert(bestInstance, candidateInstances, FALSE);
           instanceListNode = instanceListNode->next;
         }
      }

      // If we could not create any extended instances and this is not the first
      // attempt immediately after the ref graph has been extended, then it is
      // time to extend the ref graph again.
      if (!foundInstances && !firstPass)
         refInstanceListNode->doExtend = TRUE;

      // If we did not find any extended instances for this ref Graph and it is
      // the first attempt to find them after the ref graph has been extended,
      // then set empty to true so that this list node will be freed upon
      // function return.
      // We only do this when no extensions were possible on the first attempt  
      // after the ref Graph has been extended or if we do not have any
      // remaining candidate instances.  
      // Keep in mind that we keep instances that we could not extend if it is
      // possible that they could not be extended because the refGraph needed to
      // be extended first, so the candidate instance list will only be empty if
      // we have exhausted all options for all instances.
      if ((!foundInstances && firstPass) || (candidateInstances->head == NULL))
         empty = TRUE;  
      refInstanceListNode->firstPass = FALSE;

      // If we found some candidates then store them
      if (candidateInstances->head != NULL)
      {
         empty = FALSE;
         FreeInstanceList(bestSubInstanceList);
         refInstanceListNode->instanceList = candidateInstances;
      }
      else
         FreeInstanceList(candidateInstances);
   }
   else 
      empty = TRUE;

   return empty;
}


//----------------------------------------------------------------------------
// NAME:: ProcessExtendedInstances
// 
// INPUT: (Instance List *extendedInstances) - instances to extend
//        (InstanceList candidateInstances) - store sgiso matches here
//        (Substructure *bestSub) - the corresponding top-n sub
//        (ReferenceGraph *refGraph) - the corresponding ref graph
//        (Graph *fullGraph) - complete graph
//        (Parameters *parameters) - system parms
// 
// RETURN:  void
//
// PURPOSE: Take an input set of instances that have been extended and check to
//          see if they are still potential matches to their corresponding
//          top-n sub.
//----------------------------------------------------------------------------

void ProcessExtendedInstances(InstanceList *extendedInstances,
                              InstanceList *candidateInstances, 
                              Substructure *bestSub,
                              ReferenceGraph *refGraph, Graph *fullGraph, 
                              Parameters *parameters)
{
   InstanceListNode *instanceListNode;
   Instance *gInstance;
   Instance *instance;
   Substructure *newSub;
   BOOLEAN foundMatch;
   ULONG edgeIndex;
   ULONG v1;
   ULONG v2;
   ULONG index;

   if (extendedInstances == NULL)
   {
      fprintf(stderr, "extendedInstances NULL\n");
      exit(1);
   }
   instanceListNode = extendedInstances->head;

   // Look at each instance extension and keep the ones that are still subsets
   // of the top-n sub
   index = 1;
   while (instanceListNode != NULL)
   {
      instance = instanceListNode->instance;

      // create a copy of the instance and make it reference the full graph
      gInstance = CreateGraphRefInstance(instance, refGraph);
      newSub = CreateSubFromInstance(gInstance, fullGraph);

      // check to see if this instance is a subset of one of the best subs
      // first try graph match, then try sgiso

      foundMatch =
         CheckForSubgraph(newSub->definition, bestSub->definition, parameters);
      // We found this instance within (subgraph) a top n sub, so it is a keeper
      if (foundMatch && (!MemberOfInstanceList(instance, candidateInstances)))
      {
         // candidateInstances is the list of instances that are candidates for
         // further expansion
         InstanceListInsert(instance, candidateInstances, FALSE);
      }
      // this instance does not match so mark the newly added refEdge as bad
      else if(!foundMatch)
      {
         edgeIndex = instance->edges[instance->newEdge];
         v1 = refGraph->edges[edgeIndex].vertex1;
         v2 = refGraph->edges[edgeIndex].vertex2;

         // we have to ensure that this instance truly failed (and not just a
         // duplicate) before we mark the vertices and edge as invalid
         if (!VertexInSub(bestSub->definition,
                          &fullGraph->vertices[refGraph->vertices[v1].map]) || 
             !VertexInSub(bestSub->definition,
                          &fullGraph->vertices[refGraph->vertices[v2].map]))
         {
            refGraph->edges[edgeIndex].failed = TRUE;
            MarkVertices(refGraph, v1, v2);
         }
      }
      FreeSub(newSub);
      FreeInstance(gInstance);
      instanceListNode = instanceListNode->next;
      index++;
   }
}


//----------------------------------------------------------------------------
// NAME: RemoveFromList
//
// INPUT:  (RefInstanceList *refInstanceList)
//         (RefInstanceListNode **currentNode)
//
// RETURN:  void
//
// PURPOSE:  Remove a node from the ref instance list and updates the pointers.
//----------------------------------------------------------------------------

void RemoveFromList(RefInstanceList *refInstanceList,
                    RefInstanceListNode **currentNode)
{
   RefInstanceListNode *cNode;

   cNode = *currentNode;

   refInstanceList->head = cNode->next;
   FreeRefInstanceListNode(cNode);		
   *currentNode = refInstanceList->head;
}


//----------------------------------------------------------------------------
// NAME: MarkVertices
//
// INPUT: (ReferenceGraph *refGraph) - corresponding ref graph
//        (ULONG v1) - first vertex of failed edge
//        (ULONG v2) - second vertex of failed edge
//
// RETURN:  void
// PURPOSE: If a vertex in the refGraph is only connected by failed edges, then
// we mark that vertex temporarily as invalid so that we do not consider
// it for expansion until it becomes reconnected with a good edge.
//----------------------------------------------------------------------------

void MarkVertices(ReferenceGraph *refGraph, ULONG v1, ULONG v2) 
{
   BOOLEAN vertexValid;
   ReferenceVertex *refVertex;
   ReferenceEdge *refEdge;
   ULONG e;

   refVertex = &refGraph->vertices[v1];
   vertexValid = FALSE;

   //if all of the vertex's edges have failed, then mark the vertex
   //as invalid
   for (e=0; e<refVertex->numEdges; e++)
   {
      refEdge = &refGraph->edges[refVertex->edges[e]];
      if (!refEdge->failed)
      {
         vertexValid = TRUE;
         break;
      }
   }
   refVertex->vertexValid = vertexValid;

   if (v1 != v2)
   {
      refVertex = &refGraph->vertices[v2];
      vertexValid = FALSE;

      // If all of the vertex's edges have failed, then mark the vertex
      // as invalid
      for (e=0; e<refVertex->numEdges; e++)
      {
         refEdge = &refGraph->edges[refVertex->edges[e]];
         if (!refEdge->failed)
         {
            vertexValid = TRUE;
            break;
         }
      }
      refVertex->vertexValid = vertexValid;
   }
}

//----------------------------------------------------------------------------
// NAME: AddInstanceToIncrement
//
// INPUT: (Substructure *newSub) - the new substructure being added to the
//                                 increment
//        (Instance instance) - the new instance found
//        (Parameters *parameters) - system parameters
//
// RETURN:  BOOLEAN
//
// PURPOSE:  We have found a new instance of a known sub so add it to that
//           sub's instance list.  If the sub does not exist in this increment,
//           we add it.  We are not actually storing the instances, just the
//           count.  We currently do not handle overlapping vertices, so
//           instead we check the uniqueness of each vertex in an instance
//           before we add it to a sub's instance list.
//----------------------------------------------------------------------------

BOOLEAN AddInstanceToIncrement(Substructure *newSub, Instance *instance,
                               Parameters *parameters) 
{
   Increment *increment;
   SubList *subList;
   SubListNode *subListNode;
   Substructure *localSub;
   BOOLEAN found = FALSE;
   BOOLEAN overlapInOldIncrements = FALSE;
   BOOLEAN instanceAdded = FALSE;
   struct avl_table *avlTable;
	
   increment = GetCurrentIncrement(parameters);
   subList = increment->subList;
	
   // check for the sub in the current increment's sublist
   subListNode = subList->head;
   while ((subListNode != NULL) && (!found))
   {
      if (GraphMatch(newSub->definition, subListNode->sub->definition,
                     parameters->labelList, 0.0, NULL, NULL))
      {
         found = TRUE;
         // check for overlap in previous increments
         overlapInOldIncrements =
            CheckInstanceForOverlap(instance, newSub, parameters); 
         if (!overlapInOldIncrements)
         {
            // add the new instances to the sub
            subListNode->sub->numInstances++;
            AdjustMetrics(subListNode->sub, parameters);
            instanceAdded = TRUE;
            // need to reorder the subs now that we have updated the value
            // (if we care about local ordering)
         }
      }
      subListNode = subListNode->next;
   }
   // if sub not found in the current increment, then we add it
   if (!found)
   {
      // check for overlap in the previous increment
      overlapInOldIncrements =
         CheckInstanceForOverlap(instance, newSub, parameters); 
      if (!overlapInOldIncrements)
      {
         localSub = CopySub(newSub);
         localSub->numInstances = 1;
         AdjustMetrics(localSub, parameters);
         SubListInsert(localSub, subList, 0, FALSE, parameters->labelList);
         instanceAdded = TRUE;
      }
   }
   if (instanceAdded)
   {
      if (parameters->outputLevel > 3)
         PrintSub(newSub, parameters);
      avlTable = GetSubTree(newSub, parameters);
      if (avlTable == NULL)
      {
         avlTable = avl_create((avl_comparison_func *)compare_ints, NULL, NULL);
         AddInstanceVertexList(newSub, avlTable, parameters);
      }
      AddInstanceToTree(avlTable, instance);
   }

   // indicates whether the instance was added
   return(instanceAdded);
}


//----------------------------------------------------------------------------
// NAME: CheckInstanceForOverlap
//
// INPUT: (Instance *instance) - the new instance we are attempting to add to
//                               the increment
//        (Substructure *newSub) - contains the graph definition of the
//                                 instance needed for graph matching
//        (Parameters *parameters) - system parameters
//
// RETURN:  BOOLEAN
//
// PURPOSE:  Check for overlapping vertices in all previously collected
//           instances for a specific substructure.
//----------------------------------------------------------------------------

BOOLEAN CheckInstanceForOverlap(Instance *instance, Substructure *newSub,
                                Parameters *parameters)
{
   struct avl_table *avlTable;
   ULONG *vertices;
   ULONG *result;
   ULONG i;

   vertices = instance->vertices;
   avlTable = GetSubTree(newSub, parameters);
   if (avlTable == NULL)
      return FALSE;

   for(i=0; i<instance->numVertices; i++)
   {
      result = avl_find(avlTable, &vertices[i]);
      if((result != NULL) && (*result == vertices[i]))
         return TRUE;
   }
   return FALSE;
}


//---------------------------------------------------------------------------
// NAME:  CheckVertexForOverlap
//
// INPUTS:  (ULONG vertex)
//          (Substructure *sub)
//          (Parameters *parameters)
//
// RETURN:  BOOLEAN
//
// PURPOSE: Check for overlapping vertex 
//---------------------------------------------------------------------------

BOOLEAN CheckVertexForOverlap(ULONG vertex, Substructure *sub,
                              Parameters *parameters)
{
   struct avl_table *avlTable;
   ULONG *result;
		
   avlTable = GetSubTree(sub, parameters);
   if (avlTable == NULL)
      return FALSE;

   result = avl_find(avlTable, &vertex);
   if ((result != NULL) && (*result == vertex))
      return TRUE;
   else return FALSE;
}


//---------------------------------------------------------------------------
// NAME: CheckForSubgraph
//
// INPUT: (Graph *g1) - graph 1
//        (Graph *g2) - graph 2
//        (Parameters *parameters) - System parms
//
// RETURN:  BOOLEAN
//
// PURPOSE:  This is a version of the sgiso that returns TRUE as soon as the
//           first subgraph is found instead of trying to find all subgraphs.
//           That is all we care about in the boundary evaluation.
//---------------------------------------------------------------------------

BOOLEAN CheckForSubgraph(Graph *g1, Graph *g2, Parameters *parameters)
{
   ULONG v;
   ULONG v1;
   ULONG e1;
   Vertex *vertex1;
   Edge *edge1;
   InstanceList *instanceList;
   BOOLEAN *reached;
   BOOLEAN noMatches;
   BOOLEAN found;
   BOOLEAN foundMatch = FALSE;

   reached = (BOOLEAN *) malloc (sizeof (BOOLEAN) * g1->numVertices);
   if (reached == NULL)
      OutOfMemoryError ("FindInstances:reached");
   for (v1 = 0; v1 < g1->numVertices; v1++)
      reached[v1] = FALSE;

   v1 = 0; // first vertex in g1
   reached[v1] = TRUE;
   vertex1 = &g1->vertices[v1];
   instanceList = FindSingleVertexInstances (g2, vertex1, parameters);
   noMatches = FALSE;
   if (instanceList->head == NULL) // no matches to vertex1 found in g2
      noMatches = TRUE;

   while ((vertex1 != NULL) && (! noMatches) && (!foundMatch))
   {
      vertex1->used = TRUE;
      // extend by each unmarked edge involving vertex v1
      for (e1 = 0; ((e1 < vertex1->numEdges) && (! noMatches) && (!foundMatch));
           e1++)
      {
         edge1 = &g1->edges[g1->vertices[v1].edges[e1]];
         if (! edge1->used)
         {
            reached[edge1->vertex1] = TRUE;
            reached[edge1->vertex2] = TRUE;
            instanceList =
               ExtendInstancesByEdge(instanceList, g1, edge1, g2, parameters);

            // We have a set of instances from g2.  If one of them matches g1
            // then we are done.
            foundMatch = CheckForMatch(g1, instanceList, g2, parameters);
            if (instanceList->head == NULL)
               noMatches = TRUE;
            edge1->used = TRUE;
         }
      }
      if (foundMatch)
         break;

      // Find next un-used, reached vertex
      vertex1 = NULL;
      found = FALSE;
      for (v = 0; ((v < g1->numVertices) && (! found)); v++)
      {
         if ((! g1->vertices[v].used) && (reached[v]))
         {
            v1 = v;
            vertex1 = & g1->vertices[v1];
            found = TRUE;
         }
      }
   }
   free (reached);

   // set used flags to FALSE
   for (v1 = 0; v1 < g1->numVertices; v1++)
      g1->vertices[v1].used = FALSE;
   for (e1 = 0; e1 < g1->numEdges; e1++)
      g1->edges[e1].used = FALSE;

   return foundMatch;
}


//---------------------------------------------------------------------------
// NAME: CheckForMatch
//
// INPUTS:  (Graph *subGraph)
//          (InstanceList *instanceList)
//          (Graph *graph)
//          (Parameters *parameters)
//
// RETURN:  BOOLEAN
//
// PURPOSE:  A version of the function found in the sgiso code.
// Called by CheckForSubgraph.
//---------------------------------------------------------------------------

BOOLEAN CheckForMatch(Graph *subGraph, InstanceList *instanceList, Graph *graph,
                      Parameters *parameters)
{
   InstanceListNode *instanceListNode;
   Instance *instance;
   Graph *instanceGraph;
   BOOLEAN foundMatch = FALSE;
   double matchCost;
   int i = 0;

   if (instanceList != NULL) 
   {
      instanceListNode = instanceList->head;
      while (instanceListNode != NULL)
      {
         i++;
         if (instanceListNode->instance != NULL)
         {
            instance = instanceListNode->instance;
            instanceGraph = InstanceToGraph (instance, graph);
            if (GraphMatch (subGraph, instanceGraph, parameters->labelList,
                            0.0, &matchCost, NULL))
               foundMatch = TRUE;
            FreeGraph (instanceGraph);
            if(foundMatch)
               break;
         }
         instanceListNode = instanceListNode->next;
      }
   }
   return foundMatch;
}
