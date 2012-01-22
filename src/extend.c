//---------------------------------------------------------------------------
// extend.c
//
// Functions for extending a substructure.
//
// SUBDUE 5
//---------------------------------------------------------------------------

#include "subdue.h"


//---------------------------------------------------------------------------
// NAME: ExtendSub
//
// INPUTS: (Substructure *sub) - substructure to be extended
//         (Parameters *parameters)
//
// RETURN: (SubList *) - list of extended substructures
//
// PURPOSE: Return list of substructures representing extensions to
// the given substructure.  Extensions are constructed by adding an
// edge (or edge and new vertex) to each positive instance of the
// given substructure in all possible ways according to the graph.
// Matching extended instances are collected into new extended
// substructures, and all such extended substructures are returned.
// If the negative graph is present, then instances of the
// substructure in the negative graph are also collected.
//---------------------------------------------------------------------------

SubList *ExtendSub(Substructure *sub, Parameters *parameters)
{
   InstanceList *negInstanceList;
   InstanceList *newInstanceList;
   InstanceListNode *newInstanceListNode;
   Instance *newInstance;
   Substructure *newSub;
   SubList *extendedSubs;
   SubListNode *newSubListNode = NULL;
   ULONG newInstanceListIndex = 0;

   // parameters used
   Graph *posGraph = parameters->posGraph;
   Graph *negGraph = parameters->negGraph;
   LabelList *labelList = parameters->labelList;

   extendedSubs = AllocateSubList();
   newInstanceList = ExtendInstances(sub->instances, posGraph);
   negInstanceList = NULL;
   if (negGraph != NULL)
      negInstanceList = ExtendInstances(sub->negInstances, negGraph);
   newInstanceListNode = newInstanceList->head;
   while (newInstanceListNode != NULL) 
   {
      newInstance = newInstanceListNode->instance;
      if (newInstance->minMatchCost != 0.0) 
      {
         // minMatchCost=0.0 means the instance is an exact match to a
         // previously-generated sub, so a sub created from this instance
         // would be a duplicate of one already on the extendedSubs list
         newSub = CreateSubFromInstance(newInstance, posGraph);
         if (! MemberOfSubList(newSub, extendedSubs, labelList)) 
         {
            AddPosInstancesToSub(newSub, newInstance, newInstanceList, 
                                  parameters,newInstanceListIndex);
            if (negInstanceList != NULL)
               AddNegInstancesToSub(newSub, newInstance, negInstanceList, 
                                     parameters);
            // add newSub to head of extendedSubs list
            newSubListNode = AllocateSubListNode(newSub);
            newSubListNode->next = extendedSubs->head;
            extendedSubs->head = newSubListNode;
         } else FreeSub(newSub);
      }
      newInstanceListNode = newInstanceListNode->next;
      newInstanceListIndex++;
   }
   FreeInstanceList(negInstanceList);
   FreeInstanceList(newInstanceList);
   return extendedSubs;
}


//---------------------------------------------------------------------------
// NAME: ExtendInstances
//
// INPUTS: (InstanceList *instanceList) - instances to be extended
//         (Graph *graph) - graph containing substructure instances
//
// RETURN: (InstanceList *) - list of extended instances
//
// PURPOSE: Create and return a list of new instances by extending the
// given substructure's instances by one edge (or edge and new vertex)
// in all possible ways based on given graph.
//---------------------------------------------------------------------------

InstanceList *ExtendInstances(InstanceList *instanceList, Graph *graph)
{
   InstanceList *newInstanceList;
   InstanceListNode *instanceListNode;
   Instance *instance;
   Instance *newInstance;
   ULONG v;
   ULONG e;
   Vertex *vertex;
   Edge *edge;

   newInstanceList = AllocateInstanceList();
   instanceListNode = instanceList->head;
   while (instanceListNode != NULL) 
   {
      instance = instanceListNode->instance;
      MarkInstanceEdges(instance, graph, TRUE);
      for (v = 0; v < instance->numVertices; v++) 
      {
         vertex = & graph->vertices[instance->vertices[v]];
         for (e = 0; e < vertex->numEdges; e++) 
         {
            edge = & graph->edges[vertex->edges[e]];
            if (! edge->used) 
            {
               // add new instance to list
               newInstance =
                  CreateExtendedInstance(instance, instance->vertices[v],
                                         vertex->edges[e], graph);
               InstanceListInsert(newInstance, newInstanceList, TRUE);
            }
         }
      }
      MarkInstanceEdges(instance, graph, FALSE);
      instanceListNode = instanceListNode->next;
   }
   return newInstanceList;
}


//---------------------------------------------------------------------------
// NAME: CreateExtendedInstance
//
// INPUTS: (Instance *instance) - instance being extended
//         (ULONG v) - vertex in graph where new edge being added
//         (ULONG e) - edge in graph being added to instance
//         (Graph *graph) - graph containing instance and new edge
//
// RETURN: (Instance *) - new extended instance
//
// PURPOSE: Create and return a new instance, which is a copy of the
// given instance extended by one edge e along vertex v.  Edge e may
// introduce a new vertex.  Make sure that instance's vertices and
// edges arrays are kept in increasing order, which is important for
// fast instance matching.
//---------------------------------------------------------------------------

Instance *CreateExtendedInstance(Instance *instance, ULONG v, ULONG e,
                                 Graph *graph)
{
   Instance *newInstance;
   ULONG v2;
   BOOLEAN found = FALSE;
   ULONG i;

   // get edge's other vertex
   if (graph->edges[e].vertex1 == v)
      v2 = graph->edges[e].vertex2;
   else 
      v2 = graph->edges[e].vertex1;

   // check if edge's other vertex is already in instance
   for (i = 0; ((i < instance->numVertices) && (! found)); i++)
      if (instance->vertices[i] == v2)
         found = TRUE;

   // allocate memory for new instance
   if (! found)
      newInstance = AllocateInstance(instance->numVertices + 1,
                                     instance->numEdges + 1);
   else 
      newInstance = AllocateInstance(instance->numVertices,
                                     instance->numEdges + 1);

   // save pointer to what the instance was before this
   // extension (to be used as the "parent mapping")
   newInstance->parentInstance = instance;

   // set vertices and mappings of new instance
   for (i = 0; i < instance->numVertices; i++)
   {
      // copy vertices
      newInstance->vertices[i] = instance->vertices[i];

      // copy mapping
      newInstance->mapping[i].v1 = instance->mapping[i].v1;
      newInstance->mapping[i].v2 = instance->mapping[i].v2;

      // set indices to indicate indices to source and target vertices
      if (newInstance->mapping[i].v2 == graph->edges[e].vertex2)
         newInstance->mappingIndex2 = i;
      if (newInstance->mapping[i].v2 == graph->edges[e].vertex1)
         newInstance->mappingIndex1 = i;
   }

   newInstance->newVertex = VERTEX_UNMAPPED;
   if (! found) 
   {
      i = instance->numVertices;
      while ((i > 0) && (v2 < newInstance->vertices[i-1])) 
      {
         newInstance->vertices[i] = newInstance->vertices[i-1];
         newInstance->mapping[i].v1 = i;
         newInstance->mapping[i].v2 = newInstance->mapping[i-1].v2;
         // if indices moved, move mapping indices
         if (newInstance->mapping[i].v2 == graph->edges[e].vertex2)
            newInstance->mappingIndex2 = i;
         if (newInstance->mapping[i].v2 == graph->edges[e].vertex1)
            newInstance->mappingIndex1 = i;
         i--;
      }
      newInstance->vertices[i] = v2;
      newInstance->newVertex = i;
      newInstance->mapping[i].v1 = i;
      newInstance->mapping[i].v2 = v2;
      // Since this is a new vertex, need to update the index
      if (newInstance->mapping[i].v2 == graph->edges[e].vertex2)
         newInstance->mappingIndex2 = i;
      if (newInstance->mapping[i].v2 == graph->edges[e].vertex1)
         newInstance->mappingIndex1 = i;
   }

   // set edges of new instance, kept in increasing order
   for (i = 0; i < instance->numEdges; i++)
      newInstance->edges[i] = instance->edges[i];
   i = instance->numEdges;
   while ((i > 0) && (e < newInstance->edges[i-1])) 
   {
      newInstance->edges[i] = newInstance->edges[i-1];
      i--;
   }
   newInstance->edges[i] = e;
   newInstance->newEdge = i;

   return newInstance;
}


//---------------------------------------------------------------------------
// NAME: CreateSubFromInstance
//
// INPUTS: (Instance *instance) - instance
//         (Graph *graph) - graph containing instance
//
// RETURN: (Substructure *) - new substructure equivalent to instance
//
// PURPOSE: Create and return a new substructure based on the given
// instance.  Right now, the substructure is identical to the
// instance, but may be different. (*****)
//---------------------------------------------------------------------------

Substructure *CreateSubFromInstance(Instance *instance, Graph *graph)
{
   Substructure *newSub = AllocateSub();
   newSub->definition = InstanceToGraph(instance, graph);
   return newSub;
}


//---------------------------------------------------------------------------
//
// NAME: AddPosInstancesToSub
//
// INPUTS: (Substructure *sub) - substructure to collect instances
//         (Instance *subInstance) - instance of substructure
//         (InstanceList *instanceList) - instances to collect from in
//                                        positive graph
//         (Parameters *parameters)
//         (ULONG index) - index of substructure into instance list
//
// RETURN: (void)
//
// PURPOSE: Add instance from instanceList to sub's positive
// instances if the instance matches sub's definition.  If
// allowInstanceOverlap=FALSE, then instances added only if they do
// not overlap with existing instances.
//---------------------------------------------------------------------------
void AddPosInstancesToSub(Substructure *sub, Instance *subInstance,
                           InstanceList *instanceList, Parameters *parameters,
                           ULONG index)
{
   InstanceListNode *instanceListNode;
   Instance *instance;
   Graph *instanceGraph;
   double thresholdLimit;
   double matchCost;
   ULONG counter = 0;

   // parameters used
   Graph *posGraph              = parameters->posGraph;
   LabelList *labelList         = parameters->labelList;
   BOOLEAN allowInstanceOverlap = parameters->allowInstanceOverlap;
   double threshold             = parameters->threshold;

   // collect positive instances of substructure
   if (instanceList != NULL) 
   {
      sub->instances = AllocateInstanceList();
      //
      // Go ahead an insert the subInstance onto the list of instances for the
      // substructure, as it is obviously an instance.
      //
      subInstance->used = TRUE;
      InstanceListInsert(subInstance, sub->instances, FALSE);
      sub->numInstances++;
      instanceListNode = instanceList->head;
      while (instanceListNode != NULL) 
      {
         if (instanceListNode->instance != NULL) 
         {
            instance = instanceListNode->instance;
            if (allowInstanceOverlap ||
               (! InstanceListOverlap(instance, sub->instances))) 
            {
               thresholdLimit = threshold *
                                (instance->numVertices + instance->numEdges);
               instanceGraph = InstanceToGraph(instance, posGraph);
               //
               // First, if the threshold is 0.0, see if we can match on
               // just the new edge that was added.
               //
               if (threshold == 0.0)
               {
                  //
                  // To avoid processing duplicates, skip all entries
                  // before this instance (because they have been checked
                  // before in a previous call), and skip itself (because
                  // there is no point in comparing it to itself).
                  //
                  // Also, skip processing this instance if it is already
                  // matched with another substructure.
                  //
                  if ((counter > index) && (!instance->used))
                  {
                     if (NewEdgeMatch (sub->definition, subInstance, instanceGraph, 
                                       instance, parameters, thresholdLimit, 
                                       & matchCost))
                     {
                        if (matchCost < instance->minMatchCost)
                           instance->minMatchCost = matchCost;
                        instance->used = TRUE;
                        InstanceListInsert(instance, sub->instances, FALSE);
                        sub->numInstances++;
                     }
                  }
               }
               else
               {
                  if (GraphMatch(sub->definition, instanceGraph, labelList,
                                 thresholdLimit, & matchCost, NULL))
                  {
                     if (matchCost < instance->minMatchCost)
                        instance->minMatchCost = matchCost;
                     InstanceListInsert(instance, sub->instances, FALSE);
                     sub->numInstances++;
                  }
               }
               FreeGraph(instanceGraph);
            }
            counter++;
         }
         instanceListNode = instanceListNode->next;
      }
   }
}


//---------------------------------------------------------------------------
//
// NAME: AddNegInstancesToSub
//
// INPUTS: (Substructure *sub) - substructure to collect instances
//         (Instance *subInstance) - instance of substructure
//         (InstanceList *instanceList) - instances to collect from in
//                                        negative graph
//         (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Add instance from instanceList to sub's negative
// instances if the instance matches sub's definition.  If
// allowInstanceOverlap=FALSE, then instances added only if they do
// not overlap with existing instances.
//---------------------------------------------------------------------------
void AddNegInstancesToSub(Substructure *sub, Instance *subInstance,
                           InstanceList *instanceList, Parameters *parameters)
{
   InstanceListNode *instanceListNode;
   Instance *instance;
   Graph *instanceGraph;
   double thresholdLimit;
   double matchCost;

   // parameters used
   Graph *negGraph              = parameters->negGraph;
   LabelList *labelList         = parameters->labelList;
   BOOLEAN allowInstanceOverlap = parameters->allowInstanceOverlap;
   double threshold             = parameters->threshold;

   // collect negative instances of substructure
   if (instanceList != NULL) 
   {
      sub->negInstances = AllocateInstanceList();
      instanceListNode = instanceList->head;
      while (instanceListNode != NULL) 
      {
         if (instanceListNode->instance != NULL) 
         {
            instance = instanceListNode->instance;
            if (allowInstanceOverlap ||
                (! InstanceListOverlap(instance, sub->negInstances))) 
            {
               thresholdLimit = threshold *
                                (instance->numVertices + instance->numEdges);
               instanceGraph = InstanceToGraph(instance, negGraph);
               //
               // First, if the threshold is 0.0, see if we can match on
               // just the new edge that was added.
               //
               if (threshold == 0.0)
               {
                  // If instance has already been added to another substructure's
                  // list of instances, we can skip it
                  if (!instance->used)
                  {
                     if (NewEdgeMatch (sub->definition, subInstance, instanceGraph, 
                                       instance, parameters, thresholdLimit, 
                                       & matchCost))
                     {
                        if (matchCost < instance->minMatchCost)
                           instance->minMatchCost = matchCost;
                        instance->used = TRUE;
                        InstanceListInsert(instance, sub->negInstances, FALSE);
                        sub->numNegInstances++;
                     }
                  }
               } 
               else 
               {
                  if (GraphMatch(sub->definition, instanceGraph, labelList,
                                 thresholdLimit, & matchCost, NULL))
                  {
                     if (matchCost < instance->minMatchCost)
                        instance->minMatchCost = matchCost;
                     InstanceListInsert(instance, sub->negInstances, FALSE);
                     sub->numNegInstances++;
                  }
               }
               FreeGraph(instanceGraph);
            }
         }
         instanceListNode = instanceListNode->next;
      }
   }
}


//---------------------------------------------------------------------------
// NAME: RecursifySub
//
// INPUTS: (Substructure *sub) - substructure to recursify
//         (Parameters *parameters)
//
// RETURN: (Substructure *) - recursified substructure or NULL
//
// PURPOSE: Attempts to turn given substructure into a recursive
// substructure.  This is possible if two or more instances are found to be
// connected by a same-labeled edge.  Only the best such substructure is
// returned, or NULL if no such edge exists.
//
// ***** TODO: Ensure that connecting edge starts at the same vertex in
// each pair of instances.
//---------------------------------------------------------------------------

Substructure *RecursifySub(Substructure *sub, Parameters *parameters)
{
   ULONG i;
   Substructure *recursiveSub = NULL;
   Substructure *bestRecursiveSub = NULL;
   InstanceListNode *instanceListNode1;
   InstanceListNode *instanceListNode2;
   Instance *instance1;
   Instance *instance2;
   Vertex *vertex1;
   ULONG v1;
   ULONG v2Index;
   ULONG e;
   Edge *edge;
   BOOLEAN foundPair;

   // parameters used
   Graph *graph = parameters->posGraph;
   LabelList *labelList = parameters->labelList;

   // reset labels' used flag
   for (i = 0; i < labelList->numLabels; i++)
      labelList->labels[i].used = FALSE;

   // mark all instance edges
   instanceListNode1 = sub->instances->head;
   while (instanceListNode1 != NULL) 
   {
      instance1 = instanceListNode1->instance;
      MarkInstanceEdges(instance1, graph, TRUE);
      instanceListNode1 = instanceListNode1->next;
   }

   // search instance list for a connected pair
   instanceListNode1 = sub->instances->head;
   while (instanceListNode1 != NULL) 
   {
      instance1 = instanceListNode1->instance;
      for (v1 = 0; v1 < instance1->numVertices; v1++) 
      {
         vertex1 = & graph->vertices[instance1->vertices[v1]];
         for (e = 0; e < vertex1->numEdges; e++) 
         {
            edge = & graph->edges[vertex1->edges[e]];
            if ((! edge->used) && (! labelList->labels[edge->label].used)) 
            {
               // search instance list for another instance involving edge
               v2Index = edge->vertex2;
               if (edge->vertex2 == instance1->vertices[v1])
                  v2Index = edge->vertex1;
               foundPair = FALSE;
               instanceListNode2 = instanceListNode1->next;
               while ((instanceListNode2 != NULL) && (! foundPair)) 
               {
                  instance2 = instanceListNode2->instance;
                  if ((instance2 != instance1) &&
                      (InstanceContainsVertex(instance2, v2Index)))
                     foundPair = TRUE;
                  instanceListNode2 = instanceListNode2->next;
               }
               if (foundPair) 
               {
                  // connected pair of instances found; make recursive sub
                  labelList->labels[edge->label].used = TRUE;
                  recursiveSub = MakeRecursiveSub(sub, edge->label, parameters);
                  if (bestRecursiveSub == NULL)
                     bestRecursiveSub = recursiveSub;
                  else if (recursiveSub->value > bestRecursiveSub->value) 
                  {
                     FreeSub(bestRecursiveSub);
                     bestRecursiveSub = recursiveSub;
                  } 
                  else FreeSub(recursiveSub);
               }
            }
         }
      }
      instanceListNode1 = instanceListNode1->next;
   }

   // unmark all instance edges
   instanceListNode1 = sub->instances->head;
   while (instanceListNode1 != NULL) 
   {
      instance1 = instanceListNode1->instance;
      MarkInstanceEdges(instance1, graph, FALSE);
      instanceListNode1 = instanceListNode1->next;
   }

   return bestRecursiveSub;
}


//---------------------------------------------------------------------------
// NAME: MakeRecursiveSub
//
// INPUTS: (Substructure *sub) - substructure to recursify
//         (ULONG edgeLabel) - label on edge connecting instance pairs
//         (Parameters *parameters)
//
// RETURN: (Substructure *) - recursified substructure
//
// PURPOSE: Creates and returns a recursive substructure built by
// connecting chains of instances of the given substructure by the given
// edge label.
//
// ***** TODO: Ensure that connecting edge starts at the same vertex in
// each pair of instances.
//---------------------------------------------------------------------------

Substructure *MakeRecursiveSub(Substructure *sub, ULONG edgeLabel,
                               Parameters *parameters)
{
   Substructure *recursiveSub;

   // parameters used
   Graph *posGraph = parameters->posGraph;
   Graph *negGraph = parameters->negGraph;

   recursiveSub = AllocateSub();
   recursiveSub->definition = CopyGraph(sub->definition);
   recursiveSub->recursive = TRUE;
   recursiveSub->recursiveEdgeLabel = edgeLabel;
   recursiveSub->instances =
       GetRecursiveInstances(posGraph, sub->instances, sub->numInstances,
                             edgeLabel);
   recursiveSub->numInstances = CountInstances(recursiveSub->instances);
   if (negGraph != NULL) 
   {
      recursiveSub->negInstances =
         GetRecursiveInstances(negGraph, sub->negInstances,
                               sub->numNegInstances, edgeLabel);
      recursiveSub->numNegInstances =
         CountInstances(recursiveSub->negInstances);
   }
   EvaluateSub(recursiveSub, parameters);
   return recursiveSub;
}


//---------------------------------------------------------------------------
// NAME: GetRecursiveInstances
//
// INPUTS: (Graph *graph) - graph in which to look for instances
//         (InstanceList *instances) - instances in which to look for pairs
//                connected by an edge of the given label
//         (ULONG numInstances) - number of instances given
//         (ULONG recEdgeLabel) - label of edge connecting recursive instance
//                pairs
//
// RETURN: (InstanceList *) - list of recursive instances
//
// PURPOSE: Builds and returns a new instance list, where each new instance
// may contain one or more of the original instances connected by edges
// with the given edge label.  NOTE: assumes instances' edges have already
// been marked as used.
//
// ***** TODO: Ensure that connecting edge starts at the same vertex in
// each pair of instances.
//---------------------------------------------------------------------------

InstanceList *GetRecursiveInstances(Graph *graph, InstanceList *instances,
                                    ULONG numInstances, ULONG recEdgeLabel)
{
   Instance **instanceMap;
   InstanceList *recInstances;
   InstanceListNode *instanceListNode1;
   InstanceListNode *instanceListNode2;
   Instance *instance1;
   Instance *instance2;
   Vertex *vertex1;
   ULONG i, i1, i2;
   ULONG v1;
   ULONG v2Index;
   ULONG e;
   Edge *edge;

   // Allocate instance map, where instanceMap[i]=j implies the original
   // instance i is now part of possibly new instance j
   instanceMap = (Instance **) malloc(numInstances * sizeof(Instance *));
   if (instanceMap == NULL)
      OutOfMemoryError("GetRecursiveInstances:instanceMap");
   i = 0;
   instanceListNode1 = instances->head;
   while (instanceListNode1 != NULL) 
   {
      instanceMap[i] = instanceListNode1->instance;
      instanceListNode1 = instanceListNode1->next;
      i++;
   }

   // search instance list for a connected pair
   i1 = 0;
   instanceListNode1 = instances->head;
   while (instanceListNode1 != NULL) 
   {
      instance1 = instanceListNode1->instance;
      for (v1 = 0; v1 < instance1->numVertices; v1++) 
      {
         vertex1 = & graph->vertices[instance1->vertices[v1]];
         for (e = 0; e < vertex1->numEdges; e++) 
         {
            edge = & graph->edges[vertex1->edges[e]];
            if ((! edge->used) && (edge->label == recEdgeLabel)) 
            {
               // search instance list for another instance involving edge
               v2Index = edge->vertex2;
               if (edge->vertex2 == instance1->vertices[v1])
                  v2Index = edge->vertex1;
               i2 = i1 + 1;
               instanceListNode2 = instanceListNode1->next;
               while (instanceListNode2 != NULL) 
               {
                  instance2 = instanceListNode2->instance;
                  if (InstanceContainsVertex(instance2, v2Index)) 
                  {
                     // found pair, so update instance map and new instances
                     AddRecursiveInstancePair(i1, i2, instance1, instance2,
                     vertex1->edges[e], edge,
                     numInstances, instanceMap);
                  }
                  i2++;
                  instanceListNode2 = instanceListNode2->next;
               }
            }
         }
      }
      i1++;
      instanceListNode1 = instanceListNode1->next;
   }

   recInstances = CollectRecursiveInstances(instanceMap, numInstances);
   free(instanceMap);
   return recInstances;
}


//---------------------------------------------------------------------------
// NAME: AddRecursiveInstancePair
//
// INPUTS: (ULONG i1) - index of instance1 in instance map
//         (ULONG i2) - index of instance2 in instance map
//         (Instance *instance1) - first instance in connected pair to add
//         (Instance *instance2) - second instance in connected pair to add
//         (ULONG edgeIndex) - edge's index in graph
//         (Edge *edge) - edge connecting two instances
//         (ULONG numInstances) - number of original instances, i.e., length
//              of instanceMap
//         (Instance **instanceMap) - map of instances to the new instances
//              in which they reside
//
// RETURN: (void)
//
// PURPOSE: Updates the instanceMap to reflect a joining of the new
// instances containing the original instance1 and instance2, and the
// addition of the connecting edge.  Assumes no instance pair is added
// more than once.
//---------------------------------------------------------------------------

void AddRecursiveInstancePair(ULONG i1, ULONG i2,
                              Instance *instance1, Instance *instance2,
                              ULONG edgeIndex, Edge *edge,
                              ULONG numInstances, Instance **instanceMap)
{
   Instance *tmpInstance = NULL;
   ULONG i;

   if ((instanceMap[i1] == instance1) && (instanceMap[i2] == instance2)) 
   {
      // instances not yet linked to a new recursive instance
      tmpInstance = AllocateInstance(0, 0);
      AddInstanceToInstance(instance1, tmpInstance);
      AddInstanceToInstance(instance2, tmpInstance);
      AddEdgeToInstance(edgeIndex, edge, tmpInstance);
      instanceMap[i1] = tmpInstance;
      instanceMap[i2] = tmpInstance;
   } 
   else if (instanceMap[i1] == instance1) 
   {
      // instance1 to be linked to new instance at instanceMap[i2]
      AddInstanceToInstance(instance1, instanceMap[i2]);
      AddEdgeToInstance(edgeIndex, edge, instanceMap[i2]);
      instanceMap[i1] = instanceMap[i2];
   } 
   else if (instanceMap[i2] == instance2) 
   {
      // instance2 to be linked to new instance at instanceMap[i1]
      AddInstanceToInstance(instance2, instanceMap[i1]);
      AddEdgeToInstance(edgeIndex, edge, instanceMap[i1]);
      instanceMap[i2] = instanceMap[i1];
   } 
   else if (instanceMap[i1] != instanceMap[i2]) 
   {
      // both instances already belong to new different recursive instances
      AddInstanceToInstance(instanceMap[i2], instanceMap[i1]);
      AddEdgeToInstance(edgeIndex, edge, instanceMap[i1]);
      tmpInstance = instanceMap[i2];
      for (i = 0; i < numInstances; i++)
         if (instanceMap[i] == tmpInstance)
      instanceMap[i] = instanceMap[i1];
      FreeInstance(tmpInstance);
   } 
   else 
   {
      // both instances already in same new recursive instance
      AddEdgeToInstance(edgeIndex, edge, instanceMap[i1]);
   }
}


//---------------------------------------------------------------------------
// NAME: CollectRecursiveInstances
//
// INPUTS: (Instance **instanceMap) - map of instances to new instances
//              in which they reside
//         (ULONG numInstances) - number of original instances, i.e., length
//              of instanceMap
//
// RETURN: (InstanceList *) - list of new instances
//
// PURPOSE: Creates and returns a list of new instances, which are all the
// unique instances stored in the instanceMap array.
//---------------------------------------------------------------------------

InstanceList *CollectRecursiveInstances(Instance **instanceMap,
                                        ULONG numInstances)
{
   InstanceList *recInstances;
   ULONG i, j;

   recInstances = AllocateInstanceList();
   for (i = 0; i < numInstances; i++)
      if (instanceMap[i] != NULL) 
      {
         InstanceListInsert(instanceMap[i], recInstances, FALSE);
         for (j = i + 1; j < numInstances; j++)
            if (instanceMap[j] == instanceMap[i])
               instanceMap[j] = NULL;
         instanceMap[i] = NULL;
      }
   return recInstances;
}
