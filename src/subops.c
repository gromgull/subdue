//---------------------------------------------------------------------------
// subops.c
//
// Substructure and substructure list operations.
//
// SUBDUE 5
//---------------------------------------------------------------------------

#include "subdue.h"


//---------------------------------------------------------------------------
// NAME: AllocateSubListNode
//
// INPUTS: (Substructure *sub) - substructure to be stored in node
//
// RETURN: (SubListNode *) - newly allocated SubListNode
//
// PURPOSE: Allocate a new SubListNode.
//---------------------------------------------------------------------------

SubListNode *AllocateSubListNode(Substructure *sub)
{
   SubListNode *subListNode;

   subListNode = (SubListNode *) malloc(sizeof(SubListNode));
   if (subListNode == NULL)
      OutOfMemoryError("SubListNode");
   subListNode->sub = sub;
   subListNode->next = NULL;
   return subListNode;
}


//---------------------------------------------------------------------------
// NAME: FreeSubListNode
//
// INPUTS: (SubListNode *subListNode) - SubListNode to be freed
//
// RETURN: (void)
//
// PURPOSE: Deallocate memory of subListNode, including substructure
// if exists.
//---------------------------------------------------------------------------

void FreeSubListNode(SubListNode *subListNode)
{
   if (subListNode != NULL) 
   {
      FreeSub(subListNode->sub);
      free(subListNode);
   }
}


//---------------------------------------------------------------------------
// NAME: AllocateSubList
//
// INPUTS: (void)
//
// RETURN: (SubList *) - newly-allocated substructure list
//
// PURPOSE: Allocate and return an empty list to hold substructures.
//---------------------------------------------------------------------------

SubList *AllocateSubList(void)
{
   SubList *subList;

   subList = (SubList *) malloc(sizeof(SubList));
   if (subList == NULL)
      OutOfMemoryError("AllocateSubList:subList");
   subList->head = NULL;
   return subList;
}


//---------------------------------------------------------------------------
// NAME: SubListInsert
//
// INPUTS: (Substructure *sub) - substructure to be inserted
//         (SubList *subList) - list to be inserted in to
//         (ULONG max) - maximum number of substructures or different
//                       substructure values allowed on list;
//                       max = 0 means max = infinity
//         (BOOLEAN valueBased) - TRUE if list limited by different
//                                values; otherwise, limited by
//                                different substructures
//         (LabelList *labelList) - needed for checking sub equality
//
// RETURN: (void)
//
// PURPOSE: Inserts sub into subList, if not already there.  List is
// kept in decreasing order by substructure value.  If valueBased =
// TRUE, then max represents the maximum number of different valued
// substructures on the list; otherwise, max represents the maximum
// number of substructures on the list.  If sub is not inserted, then
// it is destroyed.  SubListInsert assumes given subList already
// conforms to maximums.
//---------------------------------------------------------------------------

void SubListInsert(Substructure *sub, SubList *subList, ULONG max,
                   BOOLEAN valueBased, LabelList *labelList)
{
   SubListNode *subIndex = NULL;
   SubListNode *subIndexPrevious = NULL;
   SubListNode *newSubListNode = NULL;
   ULONG numSubs = 0;
   ULONG numDiffVals = 0;
   BOOLEAN inserted = FALSE;

   newSubListNode = AllocateSubListNode(sub);

   // if subList empty, insert new sub and exit (no need to check maximums)
   if (subList->head == NULL) 
   {
      subList->head = newSubListNode;
      return;
   }

   // if sub already on subList, destroy and exit
   subIndex = subList->head;
   while ((subIndex != NULL) && (subIndex->sub->value >= sub->value))
   {
      if (subIndex->sub->value == sub->value)
      {
         if (GraphMatch(subIndex->sub->definition, sub->definition,
             labelList, 0.0, NULL, NULL))
         {
            FreeSubListNode(newSubListNode);
            return;
         }
      }
      subIndex = subIndex->next;
   }

   // sub is unique, so insert in appropriate place and check maximums
   subIndex = subList->head;
   while (subIndex != NULL) 
   {
      if (! inserted) 
      {
         if (subIndex->sub->value < sub->value) 
         {
            newSubListNode->next = subIndex;
            if (subIndexPrevious != NULL)
               subIndexPrevious->next = newSubListNode;
            else 
               subList->head = newSubListNode;
            subIndex = newSubListNode;
            inserted = TRUE;
         } 
         else if (subIndex->next == NULL) 
         {
            // Special case where the potential spot is the end of the
            // list, so go ahead and put it there, but may get removed
            // next time through the loop if boundaries are exceeded.
            subIndex->next = newSubListNode;
            inserted = TRUE;
         }
      }
    
      // update counters on number of substructures and different values
      numSubs++;
      if (subIndexPrevious == NULL)
         numDiffVals = 1;
      else if (subIndexPrevious->sub->value != subIndex->sub->value)
         numDiffVals++;
    
      // check if maximum exceeded
      if ((max > 0) && 
          (((valueBased) && (numDiffVals > max)) ||
          ((! valueBased) && (numSubs > max))) ) 
      {
         // max exceeded, so delete rest of subList from subIndex on
         if (subIndexPrevious != NULL)
            subIndexPrevious->next = NULL;
         while (subIndex != NULL) 
         {
            subIndexPrevious = subIndex;
            subIndex = subIndex->next;
            FreeSubListNode(subIndexPrevious);
         }
      } 
      else 
      {
         subIndexPrevious = subIndex;
         subIndex = subIndex->next;
      }
   }

   if (! inserted)
      FreeSubListNode(newSubListNode);
}


//---------------------------------------------------------------------------
// NAME: MemberOfSubList
//
// INPUTS: (Substructure *sub)
//         (SubList *subList)
//         (LabelList *labelList)
//
// RETURN: (BOOLEAN)
//
// PURPOSE: Check if the given substructure's definition graph exactly
// matches a definition of a substructure on the subList.
//---------------------------------------------------------------------------

BOOLEAN MemberOfSubList(Substructure *sub, SubList *subList,
                        LabelList *labelList)
{
   SubListNode *subListNode;
   BOOLEAN found = FALSE;

   if (subList != NULL) 
   {
      subListNode = subList->head;
      while ((subListNode != NULL) && (! found)) 
      {
         if (GraphMatch(sub->definition, subListNode->sub->definition,
                        labelList, 0.0, NULL, NULL))
            found = TRUE;
         subListNode = subListNode->next;
      }
   }
   return found;
}


//---------------------------------------------------------------------------
// NAME: FreeSubList
//
// INPUTS: (SubList *subList) - pointer to beginning of list to be freed
//
// RETURN:  void
//
// PURPOSE: Free memory in subList, including substructures pointed to.
//---------------------------------------------------------------------------

void FreeSubList(SubList *subList)
{
   SubListNode *subListNode1 = NULL;
   SubListNode *subListNode2 = NULL;

   if (subList != NULL) 
   {
      subListNode1 = subList->head;
      while (subListNode1 != NULL) 
      {
         subListNode2 = subListNode1;
         subListNode1 = subListNode1->next;
         FreeSub(subListNode2->sub);
         free(subListNode2);
      }
      free(subList);
   }
}


//---------------------------------------------------------------------------
// NAME: PrintSubList
//
// INPUTS: (SubList *subList) - list of substructures to print
//         (Parameters *parameters)
//
// RETURN:  void
//
// PURPOSE: Print given list of substructures.
//---------------------------------------------------------------------------

void PrintSubList(SubList *subList, Parameters *parameters)
{
   ULONG counter = 1;
   SubListNode *subListNode = NULL;

   if (subList != NULL) 
   {
      subListNode = subList->head;
      while (subListNode != NULL) 
      {
         printf("(%lu) ", counter);
         counter++;
         PrintSub(subListNode->sub, parameters);
         printf("\n");
         subListNode = subListNode->next;
      }
   }
}


//---------------------------------------------------------------------------
// NAME: AllocateSub
//
// INPUTS: void
//
// RETURN: (Substructure *) - pointer to newly allocated substructure.
//
// PURPOSE: Allocate and initialize new substructure.  A negative
// value indicates not yet computed.
//---------------------------------------------------------------------------

Substructure *AllocateSub()
{
   Substructure *sub;

   sub = (Substructure *) malloc(sizeof(Substructure));
   if (sub == NULL)
      OutOfMemoryError("substructure");
   sub->definition = NULL;
   sub->numInstances = 0;
   sub->numExamples = 0;
   sub->instances = NULL;
   sub->numNegInstances = 0;
   sub->numNegExamples = 0;
   sub->negInstances = NULL;
   sub->value = -1.0;
   sub->recursive = FALSE;
   sub->recursiveEdgeLabel = 0;

   return sub;
}


//---------------------------------------------------------------------------
// NAME: FreeSub
//
// INPUTS: (Substructure *sub) - Substructure to be freed.
//
// RETURN: void
//
// PURPOSE: Free memory used by given substructure.
//---------------------------------------------------------------------------

void FreeSub(Substructure *sub)
{
   if (sub != NULL) 
   {
      FreeGraph(sub->definition);
      FreeInstanceList(sub->instances);
      FreeInstanceList(sub->negInstances);
      free(sub);
   }
}


//---------------------------------------------------------------------------
// NAME: PrintSub
//
// INPUTS: (Substructure *sub) - substructure to print
//         (Parameters *parameters) - parameters
//
// RETURN: void
//
// PURPOSE: Print given substructure's value, number of instances,
// definition, and possibly the instances.
//---------------------------------------------------------------------------

void PrintSub(Substructure *sub, Parameters *parameters)
{
   // parameters used
   LabelList *labelList = parameters->labelList;
   ULONG outputLevel = parameters->outputLevel;

   if (sub != NULL) 
   {
      printf("Substructure: value = %.*g", NUMERIC_OUTPUT_PRECISION,
             sub->value);
      // print instance/example statistics if output level high enough
      if (outputLevel > 2) 
      {
         printf("\n                  pos instances = %lu",sub->numInstances);
         if (parameters->incremental)
            printf(", pos examples = %lu\n",(ULONG) sub->posIncrementValue);
         else
            printf(", pos examples = %lu\n",sub->numExamples);

         printf("                  neg instances = %lu",sub->numNegInstances);
         if (parameters->incremental)
            printf(", neg examples = %lu\n",(ULONG) sub->negIncrementValue);
         else
            printf(", neg examples = %lu\n",sub->numNegExamples);
      } 
      else 
      {
         if ((parameters->incremental) && (parameters->evalMethod == EVAL_SETCOVER))
            printf(", pos examples = %lu, neg examples = %lu\n",
               (ULONG) sub->posIncrementValue, (ULONG) sub->negIncrementValue);
         else
            printf(", pos instances = %lu, neg instances = %lu\n",
                   sub->numInstances, sub->numNegInstances);
      }
      // print subgraph
      if (sub->definition != NULL) 
      {
         PrintGraph(sub->definition, labelList);
      }
      if (sub->recursive) 
      {
         printf("    re ");
         PrintLabel(sub->recursiveEdgeLabel, labelList);
         printf("\n");
      }
      // print instances if output level high enough
      if (outputLevel > 2) 
      {
         printf("\n  Positive instances:\n");
         PrintPosInstanceList(sub, parameters);
         if (sub->numNegInstances > 0) 
         {
            printf("\n  Negative instances:\n");
            PrintNegInstanceList(sub, parameters);
         }
      }
   }
}


//---------------------------------------------------------------------------
// NAME: PrintNewBestSub
//
// INPUTS: (Substructure *sub) - possibly new best substructure
//         (SubList *subList) - list of best substructures
//         (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: If sub is better than the best substructure on subList, then
// print it.  This should be called only if outputLevel > 3.
//---------------------------------------------------------------------------

void PrintNewBestSub(Substructure *sub, SubList *subList,
                     Parameters *parameters)
{
   ULONG outputLevel = parameters->outputLevel;

   if ((subList->head == NULL) ||
       (sub->value > subList->head->sub->value)) 
   {
      parameters->outputLevel = 1; // turn off instance printing
      printf("\nNew best ");
      PrintSub(sub, parameters);
      printf("\n");
      parameters->outputLevel = outputLevel;
   }
}


//---------------------------------------------------------------------------
// NAME: CountSubs
//
// INPUTS: (SubList *subList) - list of substructures
//
// RETURN: (ULONG) - number of substructures in subList.
//
// PURPOSE: Return number of substructures on subList.
//---------------------------------------------------------------------------

ULONG CountSubs(SubList *subList)
{
   ULONG counter = 0;
   SubListNode *subListNode;

   if (subList != NULL) 
   {
      subListNode = subList->head;
      while (subListNode != NULL) 
      {
         counter++;
         subListNode = subListNode->next;
      }
   }
   return counter;
}


//---------------------------------------------------------------------------
// NAME: AllocateInstance
//
// INPUTS: (ULONG v) - number of vertices in instance
//         (ULONG e) - number of edges in instance
//
// RETURN: (Instance *) - pointer to newly allocated instance
//
// PURPOSE: Allocate and return space for new instance.
//---------------------------------------------------------------------------

Instance *AllocateInstance(ULONG v, ULONG e)
{
   Instance *instance;

   instance = (Instance *) malloc(sizeof(Instance));
   if (instance == NULL)
      OutOfMemoryError("AllocateInstance:instance");
   instance->numVertices = v;
   instance->numEdges = e;
   instance->vertices = NULL;
   instance->edges = NULL;
   instance->mapping = NULL;
   instance->newVertex = 0;
   instance->newEdge = 0;
   instance->mappingIndex1 = MAX_UNSIGNED_LONG;
   instance->mappingIndex2 = MAX_UNSIGNED_LONG;
   instance->used = FALSE;
   if (v > 0) 
   {
      instance->vertices = (ULONG *) malloc(sizeof(ULONG) * v);
      if (instance->vertices == NULL)
         OutOfMemoryError("AllocateInstance: instance->vertices");
      instance->mapping = (VertexMap *) malloc(sizeof(VertexMap) * v);
      if (instance->mapping == NULL)
         OutOfMemoryError("AllocateInstance: instance->mapping");
   }
   if (e > 0) 
   {
      instance->edges = (ULONG *) malloc(sizeof(ULONG) * e);
      if (instance->edges == NULL)
         OutOfMemoryError("AllocateInstance: instance->edges");
   }
   instance->minMatchCost = MAX_DOUBLE;
   instance->refCount = 0;
   instance->parentInstance = NULL;

   return instance;
}


//---------------------------------------------------------------------------
// NAME: FreeInstance
//
// INPUTS: (Instance *instance) - instance to free
//
// RETURN: (void)
//
// PURPOSE: Deallocate memory of given instance, if there are no more
// references to it.
//---------------------------------------------------------------------------

void FreeInstance(Instance *instance)
{
   if ((instance != NULL) && (instance->refCount == 0)) 
   {
      free(instance->vertices);
      free(instance->mapping);
      free(instance->edges);
      free(instance);
   }
}


//---------------------------------------------------------------------------
// NAME: AllocateInstanceListNode
//
// INPUTS: (Instance *instance) - instance to be stored in node
//
// RETURN: (InstanceListNode *) - newly allocated InstanceListNode
//
// PURPOSE: Allocate a new InstanceListNode.
//---------------------------------------------------------------------------

InstanceListNode *AllocateInstanceListNode(Instance *instance)
{
   InstanceListNode *instanceListNode;

   instanceListNode = (InstanceListNode *) malloc(sizeof(InstanceListNode));
   if (instanceListNode == NULL)
      OutOfMemoryError("AllocateInstanceListNode:InstanceListNode");
   instanceListNode->instance = instance;
   instance->refCount++;
   instanceListNode->next = NULL;
   return instanceListNode;
}


//---------------------------------------------------------------------------
// NAME: FreeInstanceListNode
//
// INPUTS: (InstanceListNode *instanceListNode)
//
// RETURN: (void)
//
// PURPOSE: Free memory used by given instance list node.
//---------------------------------------------------------------------------

void FreeInstanceListNode(InstanceListNode *instanceListNode)
{
   if (instanceListNode != NULL) 
   {
      if (instanceListNode->instance != NULL)
         instanceListNode->instance->refCount--;
      FreeInstance(instanceListNode->instance);
      free(instanceListNode);
   }
}


//---------------------------------------------------------------------------
// NAME: AllocateInstanceList
//
// INPUTS: (void)
//
// RETURN: (InstanceList *) - newly-allocated empty instance list
//
// PURPOSE: Allocate and return an empty instance list.
//---------------------------------------------------------------------------

InstanceList *AllocateInstanceList(void)
{
   InstanceList *instanceList;

   instanceList = (InstanceList *) malloc(sizeof(InstanceList));
   if (instanceList == NULL)
      OutOfMemoryError("AllocateInstanceList:instanceList");
   instanceList->head = NULL;
   return instanceList;
}


//---------------------------------------------------------------------------
// NAME: FreeInstanceList
//
// INPUTS: (InstanceList *instanceList)
//
// RETURN: (void)
//
// PURPOSE: Deallocate memory of instance list.
//---------------------------------------------------------------------------

void FreeInstanceList(InstanceList *instanceList)
{
   InstanceListNode *instanceListNode;
   InstanceListNode *instanceListNode2;

   if (instanceList != NULL) 
   {
      instanceListNode = instanceList->head;
      while (instanceListNode != NULL) 
      {
         instanceListNode2 = instanceListNode;
         instanceListNode = instanceListNode->next;
         FreeInstanceListNode(instanceListNode2);
      }
      free(instanceList);
   }
}


//---------------------------------------------------------------------------
// NAME: MarkInstanceVertices
//
// INPUTS: (Instance *instance) - instance whose vertices to set
//         (Graph *graph) - graph containing instance
//         (BOOLEAN value) - value to set edge's used flag
//
// RETURN: (void)
//
// PURPOSE: Set the used flag to the given value for each vertex in
// instance.
//---------------------------------------------------------------------------

void MarkInstanceVertices(Instance *instance, Graph *graph, BOOLEAN value)
{
   ULONG v;

   for (v = 0; v < instance->numVertices; v++)
      graph->vertices[instance->vertices[v]].used = value;
}


//---------------------------------------------------------------------------
// NAME: MarkInstanceEdges
//
// INPUTS: (Instance *instance) - instance whose edges to set
//         (Graph *graph) - graph containing instance
//         (BOOLEAN value) - value to set edge's used flag
//
// RETURN: (void)
//
// PURPOSE: Set the used flag to the given value for each edge in
// instance.
//---------------------------------------------------------------------------

void MarkInstanceEdges(Instance *instance, Graph *graph, BOOLEAN value)
{
   ULONG e;

   for (e = 0; e < instance->numEdges; e++)
      graph->edges[instance->edges[e]].used = value;
}


//---------------------------------------------------------------------------
// NAME: PrintInstance
//
// INPUTS: (Instance *instance) - instance to print
//         (ULONG vertexOffset) - offset for vertex index for this instance
//         (Graph *graph) - graph containing instance
//         (LabelList *labelList) - labels from graph
//
// RETURN: (void)
//
// PURPOSE: Print given instance.
//---------------------------------------------------------------------------

void PrintInstance(Instance *instance, ULONG vertexOffset, Graph *graph, LabelList *labelList)
{
   ULONG i;

   if (instance != NULL) 
   {
      for (i = 0; i < instance->numVertices; i++) 
      {
         printf("    ");
         PrintVertex(graph, instance->vertices[i], vertexOffset, labelList);
      }
      for (i = 0; i < instance->numEdges; i++) 
      {
         printf("    ");
         PrintEdge(graph, instance->edges[i], vertexOffset, labelList);
      }
   }
}


//---------------------------------------------------------------------------
// NAME: PrintInstanceList
//
// INPUTS: (InstanceList *instanceList) - list of instances
//         (Graph *graph) - graph containing instances
//         (LabelList *labelList) - labels used in input graph
//
// RETURN: (void)
//
// PURPOSE: Print array of instances.
//---------------------------------------------------------------------------

void PrintInstanceList(InstanceList *instanceList, Graph *graph,
                        LabelList *labelList)
{
   ULONG i = 0;
   InstanceListNode *instanceListNode;

   if (instanceList != NULL) 
   {
      instanceListNode = instanceList->head;
      while (instanceListNode != NULL) 
      {
         printf("\n  Instance %lu:\n", i + 1);
         PrintInstance(instanceListNode->instance, 0, graph, labelList);
         instanceListNode = instanceListNode->next;
         i++;
      }
   }
}


//---------------------------------------------------------------------------
// NAME: PrintPosInstanceList
//
// INPUTS: (Substructure *sub) - substructure containing positive instances
//         (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Print array of sub's positive instances.
//---------------------------------------------------------------------------

void PrintPosInstanceList(Substructure *sub, Parameters *parameters)
{
   ULONG i;
   ULONG posEgNo;
   ULONG vertexOffset = 0;
   InstanceListNode *instanceListNode;

   // parameters used
   Graph *posGraph = parameters->posGraph;
   ULONG numPosEgs = parameters->numPosEgs;
   ULONG *posEgsVertexIndices = parameters->posEgsVertexIndices;
   LabelList *labelList = parameters->labelList;

   if (sub->instances != NULL) 
   {
      instanceListNode = sub->instances->head;
      i = 1;
      while (instanceListNode != NULL) 
      {
         printf("\n  Instance %lu", i);
         if (numPosEgs > 1) 
         {
            posEgNo = InstanceExampleNumber(instanceListNode->instance,
                                            posEgsVertexIndices, numPosEgs);
            vertexOffset = posEgsVertexIndices[posEgNo - 1];
            printf(" in positive example %lu:\n", posEgNo);
         } 
         else 
            printf(":\n");
         PrintInstance(instanceListNode->instance, vertexOffset, posGraph, labelList);
         instanceListNode = instanceListNode->next;
         i++;
      }
   }
}


//---------------------------------------------------------------------------
// NAME: PrintNegInstanceList
//
// INPUTS: (Substructure *sub) - substructure containing negative instances
//         (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Print array of sub's negative instances.
//---------------------------------------------------------------------------

void PrintNegInstanceList(Substructure *sub, Parameters *parameters)
{
   ULONG i;
   ULONG negEgNo;
   ULONG vertexOffset = 0;
   InstanceListNode *instanceListNode;

   // parameters used
   Graph *negGraph = parameters->negGraph;
   ULONG numNegEgs = parameters->numNegEgs;
   ULONG *negEgsVertexIndices = parameters->negEgsVertexIndices;
   LabelList *labelList = parameters->labelList;

   if (sub->negInstances != NULL) 
   {
      instanceListNode = sub->negInstances->head;
      i = 1;
      while (instanceListNode != NULL) 
      {
         printf("\n  Instance %lu", i);
         if (numNegEgs > 1) 
         {
            negEgNo = InstanceExampleNumber(instanceListNode->instance,
                                            negEgsVertexIndices, numNegEgs);
            vertexOffset = negEgsVertexIndices[negEgNo - 1];
            printf(" in negative example %lu:\n", negEgNo);
         } 
         else 
            printf(":\n");
         PrintInstance(instanceListNode->instance, vertexOffset, negGraph, labelList);
         instanceListNode = instanceListNode->next;
         i++;
      }
   }
}


//---------------------------------------------------------------------------
// NAME: InstanceExampleNumber
//
// INPUTS: (Instance *instance) - instance to look for
//         (ULONG *egsVertexIndices) - vertex indices of graph's examples
//         (ULONG numEgs) - number of graph examples
//
// RETURN: (ULONG) - example number containing instance
//
// PURPOSE: Return which example contains the given instance.
//---------------------------------------------------------------------------

ULONG InstanceExampleNumber(Instance *instance, ULONG *egsVertexIndices,
                            ULONG numEgs)
{
   ULONG instanceVertexIndex;
   ULONG egNo;

   instanceVertexIndex = instance->vertices[0];
   egNo = 1;
   while ((egNo < numEgs) && (instanceVertexIndex >= egsVertexIndices[egNo]))
      egNo++;

   return egNo;
}


//---------------------------------------------------------------------------
// NAME: CountInstances
//
// INPUTS: (InstanceList *instanceList) - list of instances
//
// RETURN: (ULONG) - number of instances in instanceList.
//
// PURPOSE: Return number of instances in instance list.
//---------------------------------------------------------------------------

ULONG CountInstances(InstanceList *instanceList)
{
   ULONG i = 0;
   InstanceListNode *instanceListNode;

   if (instanceList != NULL) 
   {
      instanceListNode = instanceList->head;
      while (instanceListNode != NULL) 
      {
        i++;
        instanceListNode = instanceListNode->next;
      }
   }
   return i;
}


//---------------------------------------------------------------------------
// NAME: InstanceListInsert
//
// INPUTS: (Instance *instance) - instance to insert
//         (InstanceList *instanceList) - list to insert into
//         (BOOLEAN unique) - if TRUE, then instance inserted only if
//                            unique, and if not unique, deallocated
//
// RETURN: (void)
//
// PURPOSE: Insert given instance on to given instance list.  If
// unique=TRUE, then instance must not already exist on list, and if
// so, it is deallocated.  If unique=FALSE, then instance is merely
// inserted at the head of the instance list.
//---------------------------------------------------------------------------

void InstanceListInsert(Instance *instance, InstanceList *instanceList,
                        BOOLEAN unique)
{
   InstanceListNode *instanceListNode;

   if ((! unique) ||
       (unique && (! MemberOfInstanceList(instance, instanceList)))) 
   {
      instanceListNode = AllocateInstanceListNode(instance);
      instanceListNode->next = instanceList->head;
      instanceList->head = instanceListNode;
   } 
   else 
      FreeInstance(instance);
}


//---------------------------------------------------------------------------
// NAME: MemberOfInstanceList
//
// INPUTS: (Instance *instance)
//         (InstanceList *instanceList)
//
// RETURN: (BOOLEAN)
//
// PURPOSE: Check if the given instance exactly matches an instance
// already on the given instance list.
//---------------------------------------------------------------------------

BOOLEAN MemberOfInstanceList(Instance *instance, InstanceList *instanceList)
{
   InstanceListNode *instanceListNode;
   BOOLEAN found = FALSE;

   if (instanceList != NULL) 
   {
      instanceListNode = instanceList->head;
      while ((instanceListNode != NULL) && (! found)) 
      {
         if (InstanceMatch(instance, instanceListNode->instance))
            found = TRUE;
         instanceListNode = instanceListNode->next;
      }
   }
   return found;
}

//---------------------------------------------------------------------------
// NAME: InstanceMatch
//
// INPUTS: (Instance *instance1)
//         (Instance *instance2)
//
// RETURN: (BOOLEAN) - TRUE if instance1 matches instance2
//
// PURPOSE: Determine if two instances are the same by checking if
// their vertices and edges arrays are the same.  NOTE: InstanceMatch
// assumes the instances' vertices and edges arrays are in increasing
// order.
//---------------------------------------------------------------------------

BOOLEAN InstanceMatch(Instance *instance1, Instance *instance2)
{
   ULONG i;

   // check that instances have same number of vertices and edges
   if ((instance1->numVertices != instance2->numVertices) ||
       (instance1->numEdges != instance2->numEdges))
      return FALSE;

   // check that instances have same edges
   for (i = 0; i < instance1->numEdges; i++)
      if (instance1->edges[i] != instance2->edges[i])
         return FALSE;
  
   // check that instances have same vertices
   for (i = 0; i < instance1->numVertices; i++)
      if (instance1->vertices[i] != instance2->vertices[i])
         return FALSE;

   return TRUE;
}


//---------------------------------------------------------------------------
// NAME: InstanceOverlap
//
// INPUTS: (Instance *instance1)
//         (Instance *instance2)
//
// RETURN: (BOOLEAN) - TRUE if instances overlap
//
// PURPOSE: Determine if given instances overlap, i.e., share at least
// one vertex.  NOTE: instance vertices arrays are assumed to be in
// increasing order.
//---------------------------------------------------------------------------

BOOLEAN InstanceOverlap(Instance *instance1, Instance *instance2)
{
   ULONG v1;
   ULONG i = 0;
   ULONG j = 0;
   BOOLEAN overlap = FALSE;
   ULONG nv1 = instance1->numVertices;
   ULONG nv2 = instance2->numVertices;

   while ((i < nv1) && (j < nv2) && (! overlap)) 
   {
      v1 = instance1->vertices[i];
      while ((j < nv2) && (instance2->vertices[j] < v1))
         j++;
      if ((j < nv2) && (v1 == instance2->vertices[j]))
         overlap = TRUE;
      i++;
   }
   return overlap;
}


//---------------------------------------------------------------------------
// NAME: InstanceListOverlap
//
// INPUTS: (Instance *instance) - instance to check for overlap
//         (InstanceList *instanceList) - instances to check for overlap with
//                                        instance
//
// RETURN: (BOOLEAN)
//
// PURPOSE: Check if given instance overlaps at all with any instance
// in the given instance list.
//---------------------------------------------------------------------------

BOOLEAN InstanceListOverlap(Instance *instance, InstanceList *instanceList)
{
   InstanceListNode *instanceListNode;
   BOOLEAN overlap = FALSE;

   if (instanceList != NULL) 
   {
      instanceListNode = instanceList->head;
      while ((instanceListNode != NULL) && (! overlap)) 
      {
         if (instanceListNode->instance != NULL)
            if (InstanceOverlap(instance, instanceListNode->instance))
               overlap = TRUE;
         instanceListNode = instanceListNode->next;
      }
   }
   return overlap;
}


//---------------------------------------------------------------------------
// NAME: InstancesOverlap
//
// INPUTS: (InstanceList *instanceList)
//
// RETURN: (BOOLEAN) - TRUE if any pair of instances overlap
//
// PURPOSE: Check if any two instances in the given list overlap.  If
// so, return TRUE, else return FALSE.
//---------------------------------------------------------------------------

BOOLEAN InstancesOverlap(InstanceList *instanceList)
{
   InstanceListNode *instanceListNode1;
   InstanceListNode *instanceListNode2;
   BOOLEAN overlap = FALSE;

   if (instanceList != NULL) 
   {
      instanceListNode1 = instanceList->head;
      while ((instanceListNode1 != NULL) && (! overlap)) 
      {
         instanceListNode2 = instanceListNode1->next;
         while ((instanceListNode2 != NULL) && (! overlap)) 
         {
            if ((instanceListNode1->instance != NULL) &&
                (instanceListNode2->instance != NULL) &&
                (InstanceOverlap(instanceListNode1->instance,
               instanceListNode2->instance)))
            overlap = TRUE;
            instanceListNode2 = instanceListNode2->next;
         }
         instanceListNode1 = instanceListNode1->next;
      }
   }
   return overlap;
}


//---------------------------------------------------------------------------
// NAME: InstanceToGraph
//
// INPUTS: (Instance *instance) - instance to convert
//         (Graph *graph) - graph containing instance
//
// RETURN: (Graph *) - new graph equivalent to instance
//
// PURPOSE: Convert given instance to an equivalent Graph structure.
//---------------------------------------------------------------------------

Graph *InstanceToGraph(Instance *instance, Graph *graph)
{
   Graph *newGraph;
   Vertex *vertex;
   Edge *edge;
   ULONG i, j;
   ULONG v1, v2;
   BOOLEAN found1;
   BOOLEAN found2;

   v1 = 0;
   v2 = 0;
   newGraph = AllocateGraph(instance->numVertices, instance->numEdges);
 
   // convert vertices
   for (i = 0; i < instance->numVertices; i++) 
   {
      vertex = & graph->vertices[instance->vertices[i]];
      newGraph->vertices[i].label = vertex->label;
      newGraph->vertices[i].numEdges = 0;
      newGraph->vertices[i].edges = NULL;
      newGraph->vertices[i].used = FALSE;
   }

   // convert edges
   for (i = 0; i < instance->numEdges; i++) 
   {
      edge = & graph->edges[instance->edges[i]];
      // find new indices for edge vertices
      j = 0;
      found1 = FALSE;
      found2 = FALSE;
      while ((! found1) || (! found2)) 
      {
         if (instance->vertices[j] == edge->vertex1) 
         {
            v1 = j;
            found1 = TRUE;
         }
         if (instance->vertices[j] == edge->vertex2) 
         {
            v2 = j;
            found2 = TRUE;
         }
         j++;
      }
      // set new edge information
      newGraph->edges[i].vertex1 = v1;
      newGraph->edges[i].vertex2 = v2;
      newGraph->edges[i].label = edge->label;
      newGraph->edges[i].directed = edge->directed;
      newGraph->edges[i].used = FALSE;
      // add edge to appropriate vertices
      vertex = & newGraph->vertices[v1];
      vertex->numEdges++;
      vertex->edges =
         (ULONG *) realloc(vertex->edges, sizeof(ULONG) * vertex->numEdges);
      if (vertex->edges == NULL)
         OutOfMemoryError("InstanceToGraph:vertex1->edges");
      vertex->edges[vertex->numEdges - 1] = i;
      if (v1 != v2) 
      {
         vertex = & newGraph->vertices[v2];
         vertex->numEdges++;
         vertex->edges =
            (ULONG *) realloc(vertex->edges, sizeof(ULONG) * vertex->numEdges);
         if (vertex->edges == NULL)
            OutOfMemoryError("InstanceToGraph:vertex2->edges");
         vertex->edges[vertex->numEdges - 1] = i;
      }
   }
   return newGraph;
}


//---------------------------------------------------------------------------
// NAME: InstanceContainsVertex
//
// INPUTS: (Instance *instance) - instance to look in
//         (ULONG v) - vertex index to look for
//
// RETURN: (BOOLEAN) - TRUE if vertex found in instance
//
// PURPOSE: Determine if the given vertex is in the given instance.
// NOTE: instance vertices array is assumed to be in increasing order.
//---------------------------------------------------------------------------

BOOLEAN InstanceContainsVertex(Instance *instance, ULONG v)
{
   ULONG i = 0;
   BOOLEAN found = FALSE;
   ULONG nv = instance->numVertices;

   while ((i < nv) && (instance->vertices[i] < v))
      i++;
   if ((i < nv) && (instance->vertices[i] == v))
      found = TRUE;
   return found;
}


//---------------------------------------------------------------------------
// NAME: AddInstanceToInstance
//
// INPUTS: (Instance *instance1) - instance to add
//         (Instance *instance2) - instance to add to
//
// RETURN: (void)
//
// PURPOSE: Add vertices and edges of instance1 to instance2.  Duplicates
// are not added.
//---------------------------------------------------------------------------

void AddInstanceToInstance(Instance *instance1, Instance *instance2)
{
   ULONG v1, v2, e1, e2, i;
   ULONG nv2, ne2;

   // search for vertices of instance1 in instance2; if not found, then add
   // them, keeping instance2's vertices in order
   v2 = 0;
   for (v1 = 0; v1 < instance1->numVertices; v1++) 
   {
      nv2 = instance2->numVertices;
      while ((v2 < nv2) && (instance2->vertices[v2] < instance1->vertices[v1]))
         v2++;
      if ((v2 == nv2) || (instance2->vertices[v2] > instance1->vertices[v1])) 
      {
         // vertex not in instance2, so make room
         instance2->vertices =
            (ULONG *) realloc(instance2->vertices, (sizeof(ULONG) * (nv2 + 1)));
         if (instance2->vertices == NULL)
            OutOfMemoryError("AddInstanceToInstance:instance2->vertices");
         for (i = nv2; i > v2; i--)
            instance2->vertices[i] = instance2->vertices[i-1];
         instance2->vertices[v2] = instance1->vertices[v1];
         instance2->numVertices++;
      }
   }
   // insert new edges from instance1 into instance2
   e2 = 0;
   for (e1 = 0; e1 < instance1->numEdges; e1++) 
   {
      ne2 = instance2->numEdges;
      while ((e2 < ne2) && (instance2->edges[e2] < instance1->edges[e1]))
         e2++;
      if ((e2 == ne2) || (instance2->edges[e2] > instance1->edges[e1])) 
      {
         // edge not in instance2, so make room
         instance2->edges =
            (ULONG *) realloc(instance2->edges, (sizeof(ULONG) * (ne2 + 1)));
         if (instance2->edges == NULL)
            OutOfMemoryError("AddInstanceToInstance:instance2->edges");
         for (i = ne2; i > e2; i--)
            instance2->edges[i] = instance2->edges[i-1];
         instance2->edges[e2] = instance1->edges[e1];
         instance2->numEdges++;
      }
   }
}


//---------------------------------------------------------------------------
// NAME: AddEdgeToInstance
//
// INPUTS: (ULONG) edgeIndex - index of edge in graph
//         (Edge *edge) - edge to add
//         (Instance *instance2) - instance to add to
//
// RETURN: (void)
//
// PURPOSE: Add edge to instance.  Duplicate vertices or edges are not
// added.
//---------------------------------------------------------------------------

void AddEdgeToInstance(ULONG edgeIndex, Edge *edge, Instance *instance2)
{
   Instance *instance1;

   // convert edge to instance
   if (edge->vertex1 == edge->vertex2) 
   { // self-edge
      instance1 = AllocateInstance(1, 1);
      instance1->vertices[0] = edge->vertex1;
   } 
   else 
   {
      instance1 = AllocateInstance(2, 1);
      if (edge->vertex1 < edge->vertex2) 
      {
         instance1->vertices[0] = edge->vertex1;
         instance1->vertices[1] = edge->vertex2;
      } 
      else 
      {
         instance1->vertices[0] = edge->vertex2;
         instance1->vertices[1] = edge->vertex1;
      }
   }
   instance1->edges[0] = edgeIndex;
   AddInstanceToInstance(instance1, instance2);
   FreeInstance(instance1);
}


//---------------------------------------------------------------------------
// NAME:    NewEdgeMatch
//
// INPUTS:  (Graph *g1) - the graph of the substructure
//          (instance *inst1) - the substructure instance
//          (Graph *g2) - the graph of the instance
//          (instance *inst2) - the instance to be matched
//          (Parameters *parameters) - system parameters
//          (double threshold) - the threshold to be used
//          (double *cost) - cost to be returned
//
// RETURN:  (BOOLEAN) - TRUE if instances match on the 
//          newly added edge.
//
// PURPOSE: Returns TRUE if inst1 and inst2 match on the new
//          edge (and vertices) that were added.  Initially, a simple
//          check is done, then, if it matches, other attributes are
//          checked to see if it is an exact match.  Finally, if all else 
//          fails, a call to GraphMatch is made.  This is done because it
//          still could be an exact match if the graphs are just rotated.
//
// NOTE:    Currently, this function is only called if the threshold is 0.0.
//          Future performance improvements will look into how to further reduce
//          the calls to GraphMatch, which calls InexactGraphMatch, where most
//          of the SUBDUE work is performed.
//---------------------------------------------------------------------------

BOOLEAN NewEdgeMatch(Graph *g1, Instance *inst1, Graph *g2,
                     Instance *inst2, Parameters *parameters, 
                     double threshold, double *cost)
{
   VertexMap *subMapping;
   ULONG newEdge1 = inst1->newEdge;
   ULONG newEdge2 = inst2->newEdge;
   ULONG newVertex1 = inst1->newVertex;
   ULONG newVertex2 = inst2->newVertex;
   ULONG i,j;
   ULONG inst1Vertex1 = 0;
   ULONG inst1Vertex2 = 0;
   ULONG inst2Vertex1 = 0;
   ULONG inst2Vertex2 = 0;

   LabelList *labelList = parameters->labelList;

   //
   // First, see if the simple match is valid:
   //    - are the new edge labels the same?
   //    - are the directions of the edges the same?
   //    - are the new vertex labels (if new vertex added) the same?
   //
   if ((g1->edges[newEdge1].label == g2->edges[newEdge2].label) &&
       (g1->edges[newEdge1].directed == g2->edges[newEdge2].directed) &&
	   (((newVertex1 == VERTEX_UNMAPPED) && (newVertex2 == VERTEX_UNMAPPED)) ||
		((newVertex1 != VERTEX_UNMAPPED) && (newVertex2 != VERTEX_UNMAPPED) &&
		 (g1->vertices[newVertex1].label == g2->vertices[newVertex2].label))))
   {
      //
      // First, map the second instance to the parent of the first instance,
      // and then see if their mappings match.
      // (only if there is a new vertex)
      //
      if (newVertex2 != VERTEX_UNMAPPED)
      {
         UpdateMapping(inst1,inst2);
      }
      // get first instance's vertices for mapped index
      inst1Vertex1 = inst1->mapping[inst1->mappingIndex1].v1;
      inst1Vertex2 = inst1->mapping[inst1->mappingIndex2].v1;

      // get second instance's vertices for mapped index
      inst2Vertex1 = inst2->mapping[inst2->mappingIndex1].v1;
      inst2Vertex2 = inst2->mapping[inst2->mappingIndex2].v1;

      //
      // Now, see if the vertex mappings validate that they are the same
      // 
      if (((g1->edges[newEdge1].vertex1 == 
            g2->edges[newEdge2].vertex1) &&
           (g1->edges[newEdge1].vertex2 == 
            g2->edges[newEdge2].vertex2) &&
           (inst1Vertex1 == inst2Vertex1) &&
           (inst1Vertex2 == inst2Vertex2)) ||
          ((!g2->edges[newEdge2].directed) &&
           (g1->edges[newEdge1].vertex1 == 
            g2->edges[newEdge2].vertex2) &&
           (g1->edges[newEdge1].vertex2 == 
            g2->edges[newEdge2].vertex1) &&
           (inst1Vertex1 == inst2Vertex2) &&
           (inst1Vertex2 == inst2Vertex1)))
      {
         *cost = 0.0;
         return TRUE;
      }
   }
   //
   // Last resort... may need to rotate the graph to get a match...
   // Will return mapping for possible later use by the instance.
   //
   // First, create temporary holding place for mapping returned from
   // GraphMatch
   //
   subMapping = (VertexMap *) malloc(sizeof(VertexMap) * inst2->numVertices);
   if (subMapping == NULL)
      OutOfMemoryError("NewEdgeMatch: subMapping");
   
   if (GraphMatch(g1, g2, labelList, threshold, NULL, subMapping))
   {
      // Declare some variables that were not needed until now
      ULONG value;
      ULONG index = 0;
      BOOLEAN *mapSet = NULL;
      VertexMap *tempMapping;
      ULONG *sortedMapping;
      //
      // Sort the mapping returned from GraphMatch
      //
      sortedMapping = (ULONG *) malloc(sizeof(ULONG) * inst2->numVertices);
      if (sortedMapping == NULL)
         OutOfMemoryError("NewEdgeMatch: sortedMapping");
      for (i = 0; i < inst2->numVertices; i++)
         sortedMapping[subMapping[i].v1] = subMapping[i].v2;
      //
      // Then, temporarily sort the existing instance mapping
      // such that the values (v2) are in increasing order from 1..n
      // (the order returned from GraphMatch assumes that)
      //
      mapSet = (BOOLEAN *) malloc(sizeof(BOOLEAN) * inst2->numVertices);
      if (mapSet == NULL)
         OutOfMemoryError("NewEdgeMatch: mapSet");

      tempMapping = (VertexMap *) malloc(sizeof(VertexMap) * inst2->numVertices);
      if (tempMapping == NULL)
         OutOfMemoryError("NewEdgeMatch: tempMapping");

      for (i = 0; i < inst2->numVertices; i++)
         mapSet[i] = FALSE;

      for (i = 0; i < inst2->numVertices; i++)
      {
         // find what should be next mapping in the order...
         value = MAX_UNSIGNED_LONG;
         for (j = 0; j < inst2->numVertices; j++)
         {
            if ((inst2->mapping[j].v2 < value) && (!mapSet[j]))
            {
               value = inst2->mapping[j].v2;
               index = j;
            }
         }
         mapSet[index] = TRUE;
         tempMapping[i].v1 = i;
         tempMapping[i].v2 = value;
      }
      free(mapSet);
      //
      for (i=0;i<inst2->numVertices;i++)
      {
         inst2->mapping[i].v2 = tempMapping[sortedMapping[i]].v2;
      }
      //
      free(tempMapping);
      free(subMapping);
      free(sortedMapping);
      return TRUE;
   }

   free(subMapping);
   return FALSE;
}

//---------------------------------------------------------------------------
// NAME: UpdateMapping
//
// INPUTS: (Instance *instance1) - first instance (to be compared to)
//         (Instance *instance2) - second instance (to see if it belongs)
//
// RETURN: (void)
//
// PURPOSE: instance1 is the main instance and instance2 is the instance from
//          the list that is being compared to the main one to see if it
//          supposed to go on the list.  The logic below loops through
//          instance1 and its parent, find the correponding entry, then sets
//          the associated entry in instance2.  If a new vertex was added in
//          this extension (as opposed to just an edge extension), the slot
//          that has not been filled in instance2 gets the new vertex.
//
//---------------------------------------------------------------------------
void UpdateMapping(Instance *instance1, Instance *instance2)
{
   ULONG i,j;
   ULONG counter = 0;
   ULONG firstIndexValue = 0;
   ULONG secondIndexValue = 0;

   BOOLEAN found;

   // Allocate space for array of flags that indicate which map slots are set
   BOOLEAN *mapSet;
   mapSet = (BOOLEAN *) malloc(sizeof(BOOLEAN) * instance2->numVertices);
   if (mapSet == NULL)
      OutOfMemoryError("UpdateMapping: mapSet");

   // reset flags indicating whether individual map entries have been set
   for (i=0;i<instance2->numVertices;i++)
      mapSet[i] = FALSE;

   // First, need to get vertex values for mappingIndex1 and mappingIndex2
   // so that when they get rearranged below, we can reset their values.
   for (i=0;i<instance2->numVertices;i++)
   {
      if (i == instance2->mappingIndex1)
         firstIndexValue = instance2->mapping[i].v2;
      if (i == instance2->mappingIndex2)
         secondIndexValue = instance2->mapping[i].v2;
   }

   // Loop through mapping of instance2
   // "i" is used for parent instances; "j" is used for current instances
   for (i=0;i<instance2->parentInstance->numVertices;i++)
   {
      j = 0;
      found = FALSE;
      while ((j < instance1->numVertices) && (!found))
      {
         // If found corresponding entry, then set the appropriate
         // entry in instance2
         if (instance1->parentInstance->mapping[i].v2 ==
             instance1->mapping[j].v2)
         {
            instance2->mapping[j].v1 = j;
            instance2->mapping[j].v2 = instance2->parentInstance->mapping[i].v2;
            mapSet[j] = TRUE;
            //
            // Now, if this vertex was one of the mapping indices,
            // reset the appropriate index
            //
            if (instance2->mapping[j].v2 == firstIndexValue)
               instance2->mappingIndex1 = j;
            if (instance2->mapping[j].v2 == secondIndexValue)
               instance2->mappingIndex2 = j;
            counter++;
            found = TRUE;
         }
         j++;
      }
   }

   // If there is one left, it would be the new vertex
   if (counter < instance2->numVertices)
   {
      for (i=0;i<instance2->numVertices;i++)
      {
         if (!mapSet[i])
         {
            // Add new vertex in open spot
            instance2->mapping[i].v1 = i;
            instance2->mapping[i].v2 =
               instance2->vertices[instance2->newVertex];
            //
            // And move the appropriate index...
            //
            if (instance2->mapping[i].v2 == firstIndexValue)
               instance2->mappingIndex1 = i;
            if (instance2->mapping[i].v2 == secondIndexValue)
               instance2->mappingIndex2 = i;

            free(mapSet);
            return;
         }
      }
   }

   free(mapSet);
   return;
}
