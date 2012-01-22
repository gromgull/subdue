//---------------------------------------------------------------------------
// compress.c
//
// Functions related to compressing a graph with a substructure.
//
// SUBDUE 5.
//---------------------------------------------------------------------------

#include "subdue.h"

//---------------------------------------------------------------------------
// NAME: CompressGraph
//
// INPUTS: (Graph *graph) - graph to be compressed
//         (InstanceList *instanceList) - substructure instances used to
//                                        compress graph 
//         (Parameters *parameters)
//
// RETURN: (Graph *) - compressed graph
//
// PURPOSE: Returns a new graph, which is the given graph compressed
// with the given substructure instances.  "SUB" vertices replace each
// instance of the substructure, and "OVERLAP" edges are added between
// "SUB" vertices of overlapping instances.  Edges connecting to
// overlapping vertices are duplicated, one per each instance involved
// in the overlap.  Note that the "SUB" and "OVERLAP" labels are
// assumed to be the next two (respectively) labels beyond the label
// list given in the parameters, although they are not actually there
// until the graph is compressed for good at the end of the iteration.
//---------------------------------------------------------------------------

Graph *CompressGraph(Graph *graph, InstanceList *instanceList,
                     Parameters *parameters)
{
   InstanceListNode *instanceListNode;
   Instance *instance;
   ULONG instanceNo;
   ULONG numInstances;
   ULONG v, e;
   ULONG nv, ne;
   ULONG numInstanceVertices;
   ULONG numInstanceEdges;
   ULONG vertexIndex;
   ULONG subLabelIndex;
   ULONG overlapLabelIndex;
   Graph *compressedGraph;
   ULONG startVertex = 0;
   ULONG startEdge = 0;
   Increment *increment = NULL;

   // parameters used
   LabelList *labelList = parameters->labelList;
   BOOLEAN allowInstanceOverlap = parameters->allowInstanceOverlap;

   // assign "SUB" and "OVERLAP" labels an index of where they would be
   // in the label list if actually added
   subLabelIndex = labelList->numLabels;
   overlapLabelIndex = labelList->numLabels + 1;

   if (parameters->incremental)
   {
      increment = GetCurrentIncrement(parameters);
      startVertex = increment->startPosVertexIndex;
      startEdge = increment->startPosEdgeIndex;
   }

   // mark and count unique vertices and edges in graph from instances
   numInstanceVertices = 0;
   numInstanceEdges = 0;
   instanceNo = 1;
   instanceListNode = instanceList->head;

   // Count number of vertices and edges that will be compressed by instances
   while (instanceListNode != NULL)
   {
      instance = instanceListNode->instance;
      for (v = 0; v < instance->numVertices; v++)      // add in unique vertices
      {
         if ((!graph->vertices[instance->vertices[v]].used) &&
             ((!parameters->incremental) ||
              (instance->vertices[v] >= startVertex)))
         {
            numInstanceVertices++;
            graph->vertices[instance->vertices[v]].used = TRUE;
            // assign vertex to first instance it occurs in
            graph->vertices[instance->vertices[v]].map = instanceNo - 1;
         }
      }
      for (e = 0; e < instance->numEdges; e++) // add in unique edges
         if ((!graph->edges[instance->edges[e]].used) &&
             ((!parameters->incremental) ||
              (instance->edges[e] >= startEdge)))
         {
            numInstanceEdges++;
            graph->edges[instance->edges[e]].used = TRUE;
         }
      instanceNo++;
      instanceListNode = instanceListNode->next;
   }
   numInstances = instanceNo - 1;

   // allocate new graph with appropriate number of vertices and edges
   if (parameters->incremental)
   {
      nv = increment->numPosVertices - numInstanceVertices + numInstances;
      ne = increment->numPosEdges - numInstanceEdges;
   }
   else
   {
      nv = graph->numVertices - numInstanceVertices + numInstances;
      ne = graph->numEdges - numInstanceEdges;
   }
   compressedGraph = AllocateGraph(nv, ne);

   // insert SUB vertices for each instance
   vertexIndex = 0;
   for (v = 0; v < numInstances; v++) 
   {
      compressedGraph->vertices[vertexIndex].label = subLabelIndex;
      compressedGraph->vertices[vertexIndex].numEdges = 0;
      compressedGraph->vertices[vertexIndex].edges = NULL;
      compressedGraph->vertices[vertexIndex].map = VERTEX_UNMAPPED;
      compressedGraph->vertices[vertexIndex].used = FALSE;
      vertexIndex++;
   }

   // insert vertices and edges from non-compressed part of graph
   CopyUnmarkedGraph(graph, compressedGraph, vertexIndex, parameters);

   // add edges describing overlap, if appropriate (note: this will
   // unmark instance vertices)
   if (allowInstanceOverlap)
      AddOverlapEdges(compressedGraph, graph, instanceList, overlapLabelIndex,
                      startVertex, startEdge);

   // reset used flag of instances' vertices and edges
   instanceListNode = instanceList->head;
   while (instanceListNode != NULL) 
   {
      instance = instanceListNode->instance;
      MarkInstanceVertices(instance, graph, FALSE);
      MarkInstanceEdges(instance, graph, FALSE);
      instanceListNode = instanceListNode->next;
   }

   return compressedGraph;
}


//---------------------------------------------------------------------------
// NAME: AddOverlapEdges
//
// INPUTS: (Graph *compressedGraph) - compressed graph to add edges to
//         (Graph *graph) - graph being compressed
//         (InstanceList *instanceList) - substructure instances used to
//                                        compress graph
//         (ULONG overlapLabelIndex) - index into label list of "OVERLAP"
//                                     label
//         (ULONG startVertex) - index of the first vertex in the increment
//         (ULONG startEdge) - index of the first edge in the increment
//
// RETURN: (void)
//
// PURPOSE: This procedure adds edges to the compressedGraph
// describing overlapping instances of the substructure in the given
// graph.  First, if two instances overlap at all, then an undirected
// "OVERLAP" edge is added between them.  Second, if an external edge
// points to a vertex shared between multiple instances, then
// duplicate edges are added to all instances sharing the vertex.
//
// This procedure makes the following assumptions about its inputs,
// which are satisfied by CompressGraph().
//
// 1. The "SUB" vertices for the n instances are the first n vertices
// in the compressed graph's vertex array, in order corresponding to
// the instance list's order.  I.e., the ith instance in the instance
// list corresponds to compressedGraph->vertices[i-1].
//
// 2. All vertices and edges in the instances are marked (used=TRUE)
// in the graph.  Instance vertices will be unmarked as processed.
//
// 3. The vertices in the given graph are all mapped to their appropriate
// vertices in the compressedGraph.
//
// 4. For external edges pointing to vertices shared by multiple
// instances, the compressed graph already contains one such edge
// pointing to the first "SUB" vertex corresponding to the instance in
// the substructure's instance list that contains the shared vertex.
//---------------------------------------------------------------------------

void AddOverlapEdges(Graph *compressedGraph, Graph *graph,
                     InstanceList *instanceList, ULONG overlapLabelIndex,
		     ULONG startVertex, ULONG startEdge)
{
   InstanceListNode *instanceListNode1;
   InstanceListNode *instanceListNode2;
   ULONG instanceNo1;
   ULONG instanceNo2;
   Instance *instance1;
   Instance *instance2;
   ULONG v1;
   ULONG v2;
   ULONG e;
   Vertex *vertex1;
   Vertex *vertex2;
   Edge *edge1;
   Edge *overlapEdges;
   ULONG numOverlapEdges;
   ULONG totalEdges;
   ULONG edgeIndex;

   overlapEdges = NULL;
   numOverlapEdges = 0;
   // for each instance1 in substructure's instance
   instanceListNode1 = instanceList->head;
   instanceNo1 = 1;
   while (instanceListNode1 != NULL) 
   {
      instance1 = instanceListNode1->instance;
      // for each vertex in instance1
      for (v1 = 0; v1 < instance1->numVertices; v1++) 
      {
         vertex1 = &graph->vertices[instance1->vertices[v1]];
         if (vertex1->used) 
         {  // (used==TRUE) indicates unchecked for sharing
            // for each instance2 after instance1
            instanceListNode2 = instanceListNode1->next;
            instanceNo2 = instanceNo1 + 1;
            while (instanceListNode2 != NULL) 
            {
               instance2 = instanceListNode2->instance;
               // for each vertex in instance2
               for (v2 = 0; v2 < instance2->numVertices; v2++) 
               {
                  vertex2 = &graph->vertices[instance2->vertices[v2]];
                  if ((vertex1 == vertex2) &&
                      (instance1->vertices[v1] >= startVertex))
                  {  // point to same vertex, thus overlap
                     // add undirected "OVERLAP" edge between corresponding
                     // "SUB" vertices, if not already there
                     overlapEdges =
                        AddOverlapEdge(overlapEdges, &numOverlapEdges,
                                       instanceNo1 - 1, instanceNo2 - 1,
                                       overlapLabelIndex);
                     // for external edges involving vertex1, 
                     // duplicate for vertex2
                     for (e = 0; e < vertex1->numEdges; e++) 
                     {
                        edge1 = &graph->edges[vertex1->edges[e]];
                        if ((!edge1->used) &&
                            (vertex1->edges[e] >= startEdge))
                        { // edge external to instance
                           overlapEdges =
                              AddDuplicateEdges(overlapEdges, &numOverlapEdges,
                                                edge1, graph, instanceNo1 - 1,
                                                instanceNo2 - 1);
                        }
                     }
                  }
               }
               instanceListNode2 = instanceListNode2->next;
               instanceNo2++;
            }
            vertex1->used = FALSE; // i.e., done processing vertex1 for overlap
         }
      }
      instanceListNode1 = instanceListNode1->next;
      instanceNo1++;
   }

   // add overlap edges to compressedGraph
   if (numOverlapEdges > 0) 
   {
      totalEdges = compressedGraph->numEdges + numOverlapEdges;
      compressedGraph->edges =
         (Edge *) realloc(compressedGraph->edges, (totalEdges * sizeof(Edge)));
      if (compressedGraph->edges == NULL)
         OutOfMemoryError("AddOverlapEdges:compressedGraph->edges");
      edgeIndex = compressedGraph->numEdges;
      for (e = 0; e < numOverlapEdges; e++) 
      {
         StoreEdge(compressedGraph->edges, edgeIndex,
                   overlapEdges[e].vertex1, overlapEdges[e].vertex2,
                   overlapEdges[e].label, overlapEdges[e].directed,
                   overlapEdges[e].spansIncrement);
         AddEdgeToVertices(compressedGraph, edgeIndex);
         edgeIndex++;
      }
      compressedGraph->numEdges += numOverlapEdges;
      free(overlapEdges);
   }
}


//---------------------------------------------------------------------------
// NAME: AddOverlapEdge
//
// INPUTS: (Edge *overlapEdges) - edge array possibly realloc-ed to store
//                                edge
//         (ULONG *numOverlapEdgesPtr) - pointer to variable holding number
//           of total overlapping edges; this may or may not be incremented
//           depending on uniqueness of "OVERLAP" edge
//         (ULONG sub1VertexIndex) - "SUB" vertex index for first instance
//         (ULONG sub2VertexIndex) - "SUB" vertex index for second instance
//         (ULONG overlapLabelIndex) - index to "OVERLAP" label
//
// RETURN: (Edge *) - pointer to possibly realloc-ed edge array
//
// PURPOSE: If an "OVERLAP" edge does not already exist between Sub1
// and Sub2, then this function adds the edge to the given
// overlapEdges array and increments the call-by-reference variable
// numOverlapEdgesPtr.  Assumes sub1VertexIndex < sub2VertexIndex.
//---------------------------------------------------------------------------

Edge *AddOverlapEdge(Edge *overlapEdges, ULONG *numOverlapEdgesPtr,
                     ULONG sub1VertexIndex, ULONG sub2VertexIndex,
                     ULONG overlapLabelIndex)
{
   ULONG numOverlapEdges;
   ULONG e;
   BOOLEAN found;

   found = FALSE;
   numOverlapEdges = *numOverlapEdgesPtr;
   for (e = 0; ((e < numOverlapEdges) && (! found)); e++)
      if ((overlapEdges[e].vertex1 == sub1VertexIndex) &&
          (overlapEdges[e].vertex2 == sub2VertexIndex))
         found = TRUE;
   if (! found) 
   {
      overlapEdges = (Edge *) realloc(overlapEdges,
                                      ((numOverlapEdges + 1) * sizeof(Edge)));
      if (overlapEdges == NULL)
         OutOfMemoryError("AddOverlapEdge:overlapEdges");
      StoreEdge(overlapEdges, numOverlapEdges, sub1VertexIndex,
                sub2VertexIndex, overlapLabelIndex, FALSE, FALSE);
      numOverlapEdges++;
   }
   *numOverlapEdgesPtr = numOverlapEdges;
   return overlapEdges;
}


//---------------------------------------------------------------------------
// NAME: AddDuplicateEdges
//
// INPUTS: (Edge *overlapEdges) - edge array realloc-ed to store new edge
//         (ULONG *numOverlapEdgesPtr) - pointer to variable holding number
//           of total overlapping edges; will be incremented by 1, 2 or 3
//         (Edge *edge) - edge to be duplicated
//         (Graph *graph) - uncompressed graph containing edge
//         (ULONG sub1VertexIndex) - "SUB" vertex index for first instance
//         (ULONG sub2VertexIndex) - "SUB" vertex index for second instance
//
// RETURN: (Edge *) - realloc-ed edge array
//
// PURPOSE: Add duplicate edges to Sub2 based on overlapping vertex between
// Sub1 and Sub2.  The logic is complicated, so the description is offered
// as pseudocode:
//
//    if edge connects S1 to external vertex
//    then add duplicate edge connecting external vertex to S2
//    else "edge connects S1 to another (or same) vertex in S1"
//         add duplicate edge connecting S1 to S2
//         if other vertex is unmarked (overlapping and already processed)
//         then add duplicate edge connecting S2 to S2
//         if edge connects S1 to the same vertex in S1 (self edge)
//         then add duplicate edge connecting S2 to S2
//              if edge directed
//              then add duplicate edge from S2 to S1
//---------------------------------------------------------------------------

Edge *AddDuplicateEdges(Edge *overlapEdges, ULONG *numOverlapEdgesPtr,
                        Edge *edge, Graph *graph,
                        ULONG sub1VertexIndex, ULONG sub2VertexIndex)
{
   ULONG numOverlapEdges;
   ULONG v1, v2;

   numOverlapEdges = *numOverlapEdgesPtr;
   overlapEdges = (Edge *) realloc(overlapEdges,
                                   ((numOverlapEdges + 1) * sizeof(Edge)));
   if (overlapEdges == NULL)
      OutOfMemoryError("AddDuplicateEdges:overlapEdges1");

   if (graph->vertices[edge->vertex1].map != sub1VertexIndex) 
   {
      // duplicate edge from external vertex
      v1 = graph->vertices[edge->vertex1].map;
      v2 = sub2VertexIndex;
      StoreEdge(overlapEdges, numOverlapEdges, v1, v2, edge->label,
                edge->directed, edge->spansIncrement);
      numOverlapEdges++;
   } 
   else if (graph->vertices[edge->vertex2].map != sub1VertexIndex) 
   {
      // duplicate edge to an external vertex
      v1 = sub2VertexIndex;
      v2 = graph->vertices[edge->vertex2].map;
      StoreEdge(overlapEdges, numOverlapEdges, v1, v2, edge->label,
                edge->directed, edge->spansIncrement);
      numOverlapEdges++;
   } 
   else 
   {  // edge connects Sub1 to another (or same) vertex in Sub1
      // duplicate edge connecting Sub1 to Sub2
      v1 = sub1VertexIndex;
      v2 = sub2VertexIndex;
      StoreEdge(overlapEdges, numOverlapEdges, v1, v2, edge->label,
                edge->directed, edge->spansIncrement);
      numOverlapEdges++;
      // if other vertex unmarked (i.e., overlapping and already processed)
      // then duplicate edge connecting Sub2 to Sub2
      if ((! graph->vertices[edge->vertex1].used) ||
          (! graph->vertices[edge->vertex2].used)) 
      {
         overlapEdges = (Edge *) realloc(overlapEdges,
                                 ((numOverlapEdges + 1) * sizeof(Edge)));
         if (overlapEdges == NULL)
            OutOfMemoryError ("AddDuplicateEdges:overlapEdges2");
         StoreEdge(overlapEdges, numOverlapEdges, v2, v2, edge->label,
                   edge->directed, edge->spansIncrement);
         numOverlapEdges++;
      }
      // if edge connects Sub1 to the same vertex in Sub1 (self edge)
      // then add duplicate edge connecting Sub2 to Sub2
      if (edge->vertex1 == edge->vertex2) 
      {
         overlapEdges = (Edge *) realloc(overlapEdges,
                               ((numOverlapEdges + 1) * sizeof(Edge)));
         if (overlapEdges == NULL)
            OutOfMemoryError("AddDuplicateEdges:overlapEdges3");
         StoreEdge(overlapEdges, numOverlapEdges, v2, v2, edge->label,
                   edge->directed, edge->spansIncrement);
         numOverlapEdges++;
         // if edge directed
         // then add duplicate edge from S2 to S1
         if (edge->directed) 
         {
            overlapEdges = (Edge *) realloc(overlapEdges,
                                    ((numOverlapEdges + 1) * sizeof(Edge)));
            if (overlapEdges == NULL)
               OutOfMemoryError("AddDuplicateEdges:overlapEdges4");
            StoreEdge(overlapEdges, numOverlapEdges, v2, v1, edge->label,
                      edge->directed, edge->spansIncrement);
            numOverlapEdges++;
         }
      }
   }
   *numOverlapEdgesPtr = numOverlapEdges;
   return overlapEdges;
}


//---------------------------------------------------------------------------
// NAME: WriteCompressedGraphToFile
//
// INPUTS: (Substructure *sub) - substructure for compression
//         (Parameters *parameters) - contains graphs and label list
//         (ULONG iteration) - SUBDUE iteration after which compressing
//
// RETURN: void
//
// PURPOSE: Compresses the positive and negative (if defined) graphs
// with the given substructure, then writes the resulting graph to a file.
//---------------------------------------------------------------------------

void WriteCompressedGraphToFile(Substructure *sub, Parameters *parameters,
                                ULONG iteration)
{
   FILE *fp = NULL;
   Graph *compressedPosGraph;
   Graph *compressedNegGraph;
   char subLabelString[TOKEN_LEN];
   char overlapLabelString[TOKEN_LEN];
   char filename[FILE_NAME_LEN];
   Label label;

   // parameters used
   Graph *posGraph              = parameters->posGraph;
   Graph *negGraph              = parameters->negGraph;
   BOOLEAN allowInstanceOverlap = parameters->allowInstanceOverlap;
   LabelList *labelList         = parameters->labelList;

   compressedPosGraph = posGraph;
   compressedNegGraph = negGraph;

   if (sub->numInstances > 0)
      compressedPosGraph = CompressGraph(posGraph, sub->instances, parameters);
   if (sub->numNegInstances > 0)
      compressedNegGraph = CompressGraph(negGraph, sub->negInstances,
                                         parameters);

   // add "SUB" and "OVERLAP" (if used) labels to label list
   sprintf(subLabelString, "%s_%lu", SUB_LABEL_STRING, iteration);

   label.labelType = STRING_LABEL;
   label.labelValue.stringLabel = subLabelString;
   StoreLabel(&label, labelList);
   if (allowInstanceOverlap &&
       ((InstancesOverlap(sub->instances)) ||
        (InstancesOverlap(sub->negInstances)))) 
   {
      sprintf(overlapLabelString, "%s_%lu", OVERLAP_LABEL_STRING,
              iteration);
      label.labelValue.stringLabel = overlapLabelString;
      StoreLabel(&label, labelList);
   }

   if (compressedPosGraph != NULL)         // Write compressed graphs to files
   {
      sprintf(filename, "%s.cmp", parameters->inputFileName);
      fp = fopen(filename, "w");
                           // Positive examples have been combined into one file
      fprintf(fp, "XP\n");
      WriteGraphToFile(fp, compressedPosGraph, parameters->labelList,
                       0, 0, compressedPosGraph->numVertices, FALSE);
   }
   if (compressedNegGraph != NULL)
   {
      if (fp == NULL)
      {
         sprintf(filename, "%s.cmp", parameters->inputFileName);
         fp = fopen(filename, "w");
      }
                           // Negative examples have been combined into one file
      fprintf(fp, "XN\n");
      WriteGraphToFile(fp, compressedNegGraph, parameters->labelList,
                       0, 0, compressedNegGraph->numVertices, FALSE);
   }
   if (fp != NULL)
      fclose(fp);
}


//---------------------------------------------------------------------------
// NAME: CompressFinalGraphs
//
// INPUTS: (Substructure *sub) - substructure for compression
//         (Parameters *parameters) - contains graphs and label list
//         (ULONG iteration) - SUBDUE iteration (or predefined sub)
//           on which compressing
//         (BOOLEAN predefinedSub) - TRUE if compressing substructure is
//           from the predefined substructure file
//
// RETURN: void
//
// PURPOSE: Compresses the positive and negative (if defined) graphs
// with the given substructure.  Parameters->labelList is modified to
// remove any labels no longer referenced and to add "SUB_" and
// "OVERLAP_" labels, as needed.  Note that adding the "SUB_" and
// "OVERLAP_" labels must take place after the call to CompressGraph,
// because CompressGraph works under the assumption that they have not
// yet been added.  If predefinedSub=TRUE, then an addition prefix is
// added to the "SUB" and "OVERLAP" labels.
//---------------------------------------------------------------------------

void CompressFinalGraphs(Substructure *sub, Parameters *parameters,
                         ULONG iteration, BOOLEAN predefinedSub)
{
   Graph *compressedPosGraph;
   Graph *compressedNegGraph;
   char subLabelString[TOKEN_LEN];
   char overlapLabelString[TOKEN_LEN];
   Label label;
   LabelList *newLabelList;

   // parameters used
   Graph *posGraph              = parameters->posGraph;
   Graph *negGraph              = parameters->negGraph;
   BOOLEAN allowInstanceOverlap = parameters->allowInstanceOverlap;
   LabelList *labelList         = parameters->labelList;

   compressedPosGraph = posGraph;
   compressedNegGraph = negGraph;

   if (sub->numInstances > 0)
      compressedPosGraph = CompressGraph(posGraph, sub->instances, parameters);
   if (sub->numNegInstances > 0)
      compressedNegGraph = CompressGraph(negGraph, sub->negInstances,
                                         parameters);

   // add "SUB" and "OVERLAP" (if used) labels to label list
   if (parameters->incremental)
   {
      if (predefinedSub)
         sprintf(subLabelString, "%s_%s_%lu_%lu", PREDEFINED_PREFIX,
                 SUB_LABEL_STRING, iteration,
                 GetCurrentIncrementNum(parameters));
      else 
         sprintf(subLabelString, "%s_%lu_%lu", SUB_LABEL_STRING, iteration,
                 GetCurrentIncrementNum(parameters));
   }
   else
   {
      if (predefinedSub)
         sprintf(subLabelString, "%s_%s_%lu", PREDEFINED_PREFIX,
                 SUB_LABEL_STRING, iteration);
      else 
         sprintf(subLabelString, "%s_%lu", SUB_LABEL_STRING, iteration);
   }

   label.labelType = STRING_LABEL;
   label.labelValue.stringLabel = subLabelString;
   StoreLabel(& label, labelList);
   if (allowInstanceOverlap &&
       ((InstancesOverlap(sub->instances)) ||
        (InstancesOverlap(sub->negInstances)))) 
   {
      if (predefinedSub)
         sprintf(overlapLabelString, "%s_%s_%lu", PREDEFINED_PREFIX,
                 OVERLAP_LABEL_STRING, iteration);
      else 
         sprintf(overlapLabelString, "%s_%lu", OVERLAP_LABEL_STRING,
                 iteration);
      label.labelValue.stringLabel = overlapLabelString;
      StoreLabel(&label, labelList);
   }

   // reset graphs with compressed graphs
   if (sub->numInstances > 0) 
   {
      FreeGraph(parameters->posGraph);
      parameters->posGraph = compressedPosGraph;
   }
   if (sub->numNegInstances > 0) 
   {
      FreeGraph(parameters->negGraph);
      parameters->negGraph = compressedNegGraph;
   }

   // Recompute label list and MDL for positive and negative graphs.
   // This should not be done for compression using a predefined sub,
   // because compressing the label list may remove a label that a
   // later predefined sub refers to.  Since the label list may then
   // contain labels not in the positive or negative graphs, then the
   // MDL calculation will be wrong.  Recomputing the label list and
   // the graphs' MDL should be done in CompressWithPredefinedSubs
   // after all predefined subs have been processed.

   if (!predefinedSub)
   {
      // compress label list and recompute graphs' labels
      newLabelList = AllocateLabelList();
      CompressLabelListWithGraph(newLabelList, compressedPosGraph, parameters);
      if (compressedNegGraph != NULL)
         CompressLabelListWithGraph(newLabelList, compressedNegGraph,
                                    parameters);
      FreeLabelList(parameters->labelList);
      parameters->labelList = newLabelList;
 
      if (parameters->evalMethod == EVAL_MDL)
      {
         // recompute graph MDL (always necessary since label list has changed)
         parameters->posGraphDL = MDL(parameters->posGraph,
                                      newLabelList->numLabels, parameters);
         if (parameters->negGraph != NULL)
            parameters->negGraphDL = MDL(parameters->negGraph,
                                      newLabelList->numLabels, parameters);
      }
   }
}


//---------------------------------------------------------------------------
// NAME: CompressLabelListWithGraph
//
// INPUTS: (LabelList *newLabelList) - compressed label list
//         (Graph *graph) - graph used to compress label list
//         (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Adds to newLabelList only the labels present in the given
// graph.  The graph's labels are replaced with indices to the new
// label list.
//---------------------------------------------------------------------------

void CompressLabelListWithGraph(LabelList *newLabelList, Graph *graph,
                                Parameters *parameters)
{
   ULONG v, e;

   // parameters used
   LabelList *labelList = parameters->labelList;

   // add graph's vertex labels to new label list
   for (v = 0; v < graph->numVertices; v++)
      graph->vertices[v].label =
         StoreLabel(& labelList->labels[graph->vertices[v].label],
                    newLabelList);

   // add graph's edge labels to new label list
   for (e = 0; e < graph->numEdges; e++)
      graph->edges[e].label =
           StoreLabel(& labelList->labels[graph->edges[e].label], newLabelList);
}


//---------------------------------------------------------------------------
// NAME: SizeOfCompressedGraph
//
// INPUTS: (Graph *graph) - graph to be compressed
//         (InstanceList *instanceList) - instances of compressing sub
//         (Parameters *parameters)
//         (ULONG graphType)
//
// RETURN: (ULONG) - size of graph compressed with substructure
//
// PURPOSE: Computes the size (vertices+edges) of the given graph as
// if compressed using the given instances.  The compression is not
// actually done, since the compressed graph size can be determined
// computationally by subtracting the unique structure in the
// substructure instances and adding one new vertex per instance.  If
// overlap is allowed, then new edges are included to describe the
// overlap between instances and to duplicate edges to and from
// overlapping vertices.
//---------------------------------------------------------------------------

ULONG SizeOfCompressedGraph(Graph *graph, InstanceList *instanceList,
                            Parameters *parameters, ULONG graphType)
{
   ULONG size;
   InstanceListNode *instanceListNode;
   Instance *instance;
   ULONG v, e;
   BOOLEAN allowInstanceOverlap = parameters->allowInstanceOverlap;

   if (parameters->incremental)
      size = IncrementSize(parameters, GetCurrentIncrementNum(parameters),
                           graphType);
   else size = GraphSize(graph);

   if (instanceList != NULL) 
   {
      instanceListNode = instanceList->head;
      if (allowInstanceOverlap) 
      {
         // reduce size by amount of unique structure, which is marked
         while (instanceListNode != NULL) 
         {
            size++; // new "SUB" vertex of instance
            instance = instanceListNode->instance;
            // subtract unique vertices
            for (v = 0; v < instance->numVertices; v++)
               if (!graph->vertices[instance->vertices[v]].used) 
               {
                  size--;
                  graph->vertices[instance->vertices[v]].used = TRUE;
               }
            for (e = 0; e < instance->numEdges; e++)   // subtract unique edges
               if (!graph->edges[instance->edges[e]].used) 
               {
                  size--;
                  graph->edges[instance->edges[e]].used = TRUE;
               }
            instanceListNode = instanceListNode->next;
         }
         // increase size by number of overlap edges (assumes marked instances)
         size += NumOverlapEdges(graph, instanceList);
         // reset used flag of instances' vertices and edges
         instanceListNode = instanceList->head;
         while (instanceListNode != NULL) 
         {
            instance = instanceListNode->instance;
            MarkInstanceVertices(instance, graph, FALSE);
            MarkInstanceEdges(instance, graph, FALSE);
            instanceListNode = instanceListNode->next;
         }
      }
      else
      {
         // no overlap, so just subtract size of instances
         while (instanceListNode != NULL) 
         {
            size++; // new "SUB" vertex of instance
            instance = instanceListNode->instance;
            size -= (instance->numVertices + instance->numEdges);
            instanceListNode = instanceListNode->next;
         }
      }
   }
   return size;
}


//---------------------------------------------------------------------------
// NAME: NumOverlapEdges
//
// INPUTS: (Graph *graph) - graph being "compressed" by substructure
//         (InstanceList *instanceList) - substructure instances
//
// RETURN: (ULONG) - number of "OVERLAP" and duplicate edges needed
//
// PURPOSE: Computes the number of "OVERLAP" and duplicate edges that
// would be needed to compress the graph with the substructure
// instances.  If two instances overlap at all, then an undirected
// "OVERLAP" edge is added between them.  If an external edge points
// to a vertex shared between multiple instances, then duplicate edges
// are added to all instances sharing the vertex.
//
// This procedure assumes all vertices and edges in the instances are
// marked (used=TRUE) in the graph.  Instance vertices will be
// unmarked as processed.
//---------------------------------------------------------------------------

ULONG NumOverlapEdges(Graph *graph, InstanceList *instanceList)
{
   InstanceListNode *instanceListNode1;
   InstanceListNode *instanceListNode2;
   ULONG instanceNo1;
   ULONG instanceNo2;
   Instance *instance1;
   Instance *instance2;
   ULONG v1;
   ULONG v2;
   ULONG e;
   Vertex *vertex1;
   Vertex *vertex2;
   Edge *edge1;
   Edge *overlapEdges;
   ULONG numOverlapEdges;
   ULONG overlapLabelIndex;

   overlapLabelIndex = 0; // bogus value never used since graph not compressed
   overlapEdges = NULL;
   numOverlapEdges = 0;
   // for each instance1 in substructure's instance
   instanceListNode1 = instanceList->head;
   instanceNo1 = 1;
   while (instanceListNode1 != NULL) 
   {
      instance1 = instanceListNode1->instance;
      // for each vertex in instance1
      for (v1 = 0; v1 < instance1->numVertices; v1++) 
      {
         vertex1 = & graph->vertices[instance1->vertices[v1]];
         if (vertex1->used) 
         { // (used==TRUE) indicates unchecked for sharing
            // for each instance2 after instance1
            instanceListNode2 = instanceListNode1->next;
            instanceNo2 = instanceNo1 + 1;
            while (instanceListNode2 != NULL) 
            {
               instance2 = instanceListNode2->instance;
               // for each vertex in instance2
               for (v2 = 0; v2 < instance2->numVertices; v2++) 
               {
                  vertex2 = & graph->vertices[instance2->vertices[v2]];
                  if (vertex1 == vertex2) 
                  { // point to same vertex, thus overlap
                     // add undirected "OVERLAP" edge between corresponding
                     // "SUB" vertices, if not already there
                     overlapEdges =
                        AddOverlapEdge(overlapEdges, & numOverlapEdges,
                                       instanceNo1 - 1, instanceNo2 - 1,
                                       overlapLabelIndex);
                     // for external edges involving vertex1, 
                     // duplicate for vertex2
                     for (e = 0; e < vertex1->numEdges; e++) 
                     {
                        edge1 = & graph->edges[vertex1->edges[e]];
                        if (! edge1->used) 
                        { // edge external to instance
                           overlapEdges =
                              AddDuplicateEdges(overlapEdges, & numOverlapEdges,
                                                edge1, graph,
                                                instanceNo1 - 1, instanceNo2 - 1);
                        }
                     }
                  }
               }
               instanceListNode2 = instanceListNode2->next;
               instanceNo2++;
            }
            vertex1->used = FALSE; // i.e., done processing vertex1 for overlap
         }
      }
      instanceListNode1 = instanceListNode1->next;
      instanceNo1++;
   }
   free (overlapEdges);
   return numOverlapEdges;
}


//---------------------------------------------------------------------------
// NAME: RemovePosEgsCovered
//
// INPUTS: (Substructure *sub) - substructure whose instances are to be removed
//         (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: This is basically compression for the set-covering
// evaluation method.  Creates a new positive graph containing only
// those examples not covered (in whole or in part) by an instance of
// the substructure.  The parameters related to the positive graph and
// the label list are updated accordingly.
//---------------------------------------------------------------------------

void RemovePosEgsCovered(Substructure *sub, Parameters *parameters)
{
   InstanceList *instanceList;
   InstanceListNode *instanceListNode;
   ULONG posEg;
   ULONG posEgStartVertexIndex;
   ULONG posEgEndVertexIndex;
   ULONG instanceVertexIndex;
   BOOLEAN found;
   ULONG e;
   Graph *newPosGraph;
   ULONG newNumPosEgs;
   ULONG newNumVertices;
   ULONG newNumEdges;
   ULONG vertexOffset;
   ULONG *newPosEgsVertexIndices;
   LabelList *newLabelList;

   // parameters used
   Graph *posGraph            = parameters->posGraph;
   Graph *negGraph            = parameters->negGraph;
   ULONG numPosEgs            = parameters->numPosEgs;
   ULONG *posEgsVertexIndices = parameters->posEgsVertexIndices;
 
   // if no instances, then no changes to positive graph
   if (sub->instances == NULL)
      return;

   newNumPosEgs = 0;
   newNumVertices = 0;
   newNumEdges = 0;
   newPosEgsVertexIndices = NULL;
   instanceList = sub->instances;
   // for each example, look for a covering instance
   for (posEg = 0; posEg < numPosEgs; posEg++) 
   {
      // find example's starting and ending vertex indices
      posEgStartVertexIndex = posEgsVertexIndices[posEg];
      if (posEg < (numPosEgs - 1))
         posEgEndVertexIndex = posEgsVertexIndices[posEg + 1] - 1;
      else 
         posEgEndVertexIndex = posGraph->numVertices - 1;
      // look for an instance whose vertices are in range
      instanceListNode = instanceList->head;
      found = FALSE;
      while ((instanceListNode != NULL) && (! found)) 
      {
         // can check any instance vertex, so use the first
         instanceVertexIndex = instanceListNode->instance->vertices[0];
         if ((instanceVertexIndex >= posEgStartVertexIndex) &&
             (instanceVertexIndex <= posEgEndVertexIndex)) 
         {
            // found an instance covering this example
            found = TRUE;
         }
         instanceListNode = instanceListNode->next;
      }
      if (found) 
      {
         // mark vertices and edges of example (note: these will not be
         // unmarked, because this posGraph will soon be de-allocated)
         MarkExample(posEgStartVertexIndex, posEgEndVertexIndex,
                     posGraph, TRUE);
      } 
      else 
      {
         newNumPosEgs++;
         vertexOffset = newNumVertices;
         newPosEgsVertexIndices = AddVertexIndex(newPosEgsVertexIndices,
                                                 newNumPosEgs, vertexOffset);
         newNumVertices += (posEgEndVertexIndex - posEgStartVertexIndex + 1);
      }
   }
   // count number of edges in examples left uncovered
   for (e = 0; e < posGraph->numEdges; e++)
      if (! posGraph->edges[e].used)
         newNumEdges++;

   // create new positive graph and copy unmarked part of old
   newPosGraph = AllocateGraph(newNumVertices, newNumEdges);
   CopyUnmarkedGraph(posGraph, newPosGraph, 0, parameters);

   // compress label list and recompute graphs' labels
   newLabelList = AllocateLabelList();
   CompressLabelListWithGraph(newLabelList, newPosGraph, parameters);
   if (negGraph != NULL)
      CompressLabelListWithGraph(newLabelList, negGraph, parameters);
   FreeLabelList(parameters->labelList);
   parameters->labelList = newLabelList;

   // reset positive graph stats and recompute MDL (only needed for printing)
   FreeGraph(parameters->posGraph);
   parameters->posGraph = newPosGraph;
   free(parameters->posEgsVertexIndices);
   parameters->posEgsVertexIndices = newPosEgsVertexIndices;
   parameters->numPosEgs = newNumPosEgs;
   if (parameters->evalMethod == EVAL_MDL)
      parameters->posGraphDL = MDL(newPosGraph, newLabelList->numLabels,
                                   parameters);
}


//---------------------------------------------------------------------------
// NAME: MarkPosEgsCovered
//
// INPUTS: (Substructure *sub) - substructure whose instances are to be marked
//         (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: For the set-covering evaluate method, mark vertices covered by
//          the substructure so they will not be written to the compressed
//          file.
//---------------------------------------------------------------------------

void MarkPosEgsCovered(Substructure *sub, Parameters *parameters)
{
   InstanceList *instanceList;
   InstanceListNode *instanceListNode;
   ULONG posEg;
   ULONG posEgStartVertexIndex;
   ULONG posEgEndVertexIndex;
   ULONG instanceVertexIndex;
   BOOLEAN found;

   // parameters used
   Graph *posGraph            = parameters->posGraph;
   ULONG numPosEgs            = parameters->numPosEgs;
   ULONG *posEgsVertexIndices = parameters->posEgsVertexIndices;
 
   // if no instances, then no changes to positive graph
   if (sub->instances == NULL)
      return;

   instanceList = sub->instances;
   // for each example, look for a covering instance
   for (posEg = 0; posEg < numPosEgs; posEg++) 
   {
      // find example's starting and ending vertex indices
      posEgStartVertexIndex = posEgsVertexIndices[posEg];
      if (posEg < (numPosEgs - 1))
         posEgEndVertexIndex = posEgsVertexIndices[posEg + 1] - 1;
      else 
         posEgEndVertexIndex = posGraph->numVertices - 1;
      // look for an instance whose vertices are in range
      instanceListNode = instanceList->head;
      found = FALSE;
      while ((instanceListNode != NULL) && (! found)) 
      {
         // can check any instance vertex, so use the first
         instanceVertexIndex = instanceListNode->instance->vertices[0];
         if ((instanceVertexIndex >= posEgStartVertexIndex) &&
             (instanceVertexIndex <= posEgEndVertexIndex)) 
         {
            // found an instance covering this example
            found = TRUE;
         }
         instanceListNode = instanceListNode->next;
      }
      if (found) 
      {
         // mark vertices and edges of example (note: these will not be
         // unmarked, because this posGraph will soon be de-allocated)
         MarkExample(posEgStartVertexIndex, posEgEndVertexIndex,
                     posGraph, TRUE);
      } 
   }
}


//---------------------------------------------------------------------------
// NAME: WriteUpdatedGraphToFile
//
// INPUTS: (Substructure *sub) - substructure whose instances to be removed
//         (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Write compressed graphs to file, including separators for examples.
//---------------------------------------------------------------------------

void WriteUpdatedGraphToFile(Substructure *sub, Parameters *parameters)
{
   FILE *fp;
   char filename[FILE_NAME_LEN];
   ULONG example;
   ULONG numExamples;
   ULONG start;
   ULONG finish;

   RemovePosEgsCovered(sub, parameters);

   sprintf(filename, "%s.cmp", parameters->inputFileName);
   if (parameters->posGraph != NULL)       // Write compressed graphs to files
   {
      numExamples = parameters->numPosEgs;
      fp = fopen(filename, "w");

      // The indices of each example need to be renumbered to start at 1
      for (example = 0; example < numExamples; example++)
      {
         start = parameters->posEgsVertexIndices[example];
         if (example < (numExamples - 1))
            finish = parameters->posEgsVertexIndices[example + 1];
         else 
            finish = parameters->posGraph->numVertices;
         fprintf(fp, "XP\n");
         WriteGraphToFile(fp, parameters->posGraph, parameters->labelList, 0,
                          start, finish, FALSE);
      }
      fclose(fp);
   }

   if (parameters->negGraph != NULL)
   {
      numExamples = parameters->numNegEgs;
      fp = fopen(filename, "a");

      // The indices of each example need to be renumbered to start at 1
      for (example = 0; example < numExamples; example++)
      {
         start = parameters->negEgsVertexIndices[example];
         if (example < (numExamples - 1))
            finish = parameters->negEgsVertexIndices[example + 1];
         else 
            finish = parameters->negGraph->numVertices;
         fprintf(fp, "XN\n");
         WriteGraphToFile(fp, parameters->negGraph, parameters->labelList, 0,
                          start, finish, FALSE);
      }
      fclose(fp);
   }
}


//---------------------------------------------------------------------------
// NAME: WriteUpdatedIncToFile
//
// INPUTS: (Substructure *sub) - substructure whose instances to be removed
//         (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Write compressed graphs to file, including separators for examples.
//---------------------------------------------------------------------------

void WriteUpdatedIncToFile(Substructure *sub, Parameters *parameters)
{
   FILE *fp;
   char filename[FILE_NAME_LEN];
   ULONG example;
   ULONG numExamples;
   ULONG start = 0;
   ULONG finish;
   ULONG i;
   ULONG first = 0;
   Increment *increment = GetCurrentIncrement(parameters);

   if (parameters->incremental)
      first = increment->startPosVertexIndex;
   MarkPosEgsCovered(sub, parameters);

   sprintf(filename, "%s-com_%lu.g", parameters->inputFileName,
                     GetCurrentIncrementNum(parameters));
   if (parameters->posGraph != NULL)       // Write compressed graphs to files
   {
      numExamples = parameters->numPosEgs;
      fp = fopen(filename, "w");

      // The indices of each example need to be renumbered to start at 1
      for (example = 0; example < numExamples; example++)
      {
         start = parameters->posEgsVertexIndices[example];
         if (start >= first)
	 {
            if (example < (numExamples - 1))
               finish = parameters->posEgsVertexIndices[example + 1];
            else 
               finish = parameters->posGraph->numVertices;
            // Only write positive examples not covered by substructure
            if (!parameters->posGraph->vertices[start].used)
            {
               fprintf(fp, "XP\n");
               WriteGraphToFile(fp, parameters->posGraph, parameters->labelList,
                                0, start, finish, FALSE);
            }
         }
      }
      fclose(fp);
   }

   if (parameters->incremental)
      first = increment->startNegVertexIndex;

   if (parameters->negGraph != NULL)
   {
      numExamples = parameters->numNegEgs;
      fp = fopen(filename, "a");

      // The indices of each example need to be renumbered to start at 1
      for (example = 0; example < numExamples; example++)
      {
         start = parameters->negEgsVertexIndices[example];
         if (start >= first)
         {
            if (example < (numExamples - 1))
               finish = parameters->negEgsVertexIndices[example + 1];
            else 
               finish = parameters->negGraph->numVertices;
            fprintf(fp, "XN\n");
            WriteGraphToFile(fp, parameters->negGraph, parameters->labelList,
                             0, start, finish, FALSE);
         }
      }
      fclose(fp);
   }

   // Unmark covered vertices
   for (i=start; i<parameters->posGraph->numVertices; i++)
      parameters->posGraph->vertices[i].used = FALSE; 
}


//---------------------------------------------------------------------------
// NAME: MarkExample
//
// INPUTS: (ULONG egStartVertexIndex) - starting vertex of example
//         (ULONG egEndVertexIndex) - ending vertex of example
//         (Graph *graph) - graph containing example
//         (BOOLEAN value) - value for used flag of example vertices/edges
//
// RETURN: (void)
//
// PURPOSE: Sets the used flag to value for all vertices and edges
// comprising the example whose range of vertices is given.
//---------------------------------------------------------------------------

void MarkExample(ULONG egStartVertexIndex, ULONG egEndVertexIndex,
                 Graph *graph, BOOLEAN value)
{
   ULONG v;
   ULONG e;
   Vertex *vertex;

   for (v = egStartVertexIndex; v <= egEndVertexIndex; v++) 
   {
      vertex = & graph->vertices[v];
      vertex->used = value;
      for (e = 0; e < vertex->numEdges; e++)
         graph->edges[vertex->edges[e]].used = value;
   }
}


//---------------------------------------------------------------------------
// NAME: CopyUnmarkedGraph
//
// INPUTS: (Graph *g1) - graph to copy unused structure from
//         (Graph *g2) - graph to copy to
//         (ULONG vertexIndex) - index into g2's vertex array where to
//           start copying unused vertices from g1
//         (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Copy unused vertices and edges from g1 to g2, starting at
// vertexIndex of g2's vertex array.  Ensures that copied edges map to
// correct vertices in g2.
//---------------------------------------------------------------------------

void CopyUnmarkedGraph(Graph *g1, Graph *g2, ULONG vertexIndex,
                       Parameters *parameters)
{
   ULONG v, e;
   ULONG v1, v2;
   ULONG edgeIndex;
   ULONG vertexOffset;
   ULONG edgeOffset;
   Increment *increment = NULL;

   if (parameters->incremental)
   {
      increment = GetCurrentIncrement(parameters);
      vertexOffset = increment->startPosVertexIndex;

      // Copy the newly compressed increment vertices to the new graph
      for (v = 0; v < increment->numPosVertices; v++)
         if (!g1->vertices[vertexOffset + v].used)
         {
            g2->vertices[vertexIndex].label =
               g1->vertices[vertexOffset + v].label;
            // We have to tell the data generation module how to
            // link to this data from the next increment
            g2->vertices[vertexIndex].numEdges = 0;
            g2->vertices[vertexIndex].edges = NULL;
            g2->vertices[vertexIndex].map = VERTEX_UNMAPPED;
            g2->vertices[vertexIndex].used = FALSE;
            g1->vertices[vertexOffset + v].map = vertexIndex;
            vertexIndex++;
         }
 
      // Copy the newly compressed increment edges to the new graph
      edgeOffset = increment->startPosEdgeIndex;
      edgeIndex = 0;
      for (e = 0; e < increment->numPosEdges; e++)
         if (!g1->edges[edgeOffset + e].used)
         {
            if (g1->edges[edgeOffset + e].spansIncrement)
               g2->numEdges = g2->numEdges - 1;
            else
            {
               v1 = g1->vertices[g1->edges[edgeOffset + e].vertex1].map;
               v2 = g1->vertices[g1->edges[edgeOffset + e].vertex2].map;
               StoreEdge(g2->edges, edgeIndex, v1, v2,
	                 g1->edges[edgeOffset + e].label,
                         g1->edges[edgeOffset + e].directed,
                         g1->edges[edgeOffset + e].spansIncrement);
               AddEdgeToVertices(g2, edgeIndex);
               edgeIndex++;
            }
         }
   }
   else
   {
      // copy unused vertices from g1 to g2
      for (v = 0; v < g1->numVertices; v++)
         if (! g1->vertices[v].used) 
         {
            g2->vertices[vertexIndex].label = g1->vertices[v].label;
            g2->vertices[vertexIndex].numEdges = 0;
            g2->vertices[vertexIndex].edges = NULL;
            g2->vertices[vertexIndex].map = VERTEX_UNMAPPED;
            g2->vertices[vertexIndex].used = FALSE;
            g1->vertices[v].map = vertexIndex;
            vertexIndex++;
         }
      // copy unused edges from g1 to g2
      edgeIndex = 0;
      for (e = 0; e < g1->numEdges; e++)
         if (! g1->edges[e].used) 
         {
            v1 = g1->vertices[g1->edges[e].vertex1].map;
            v2 = g1->vertices[g1->edges[e].vertex2].map;
            StoreEdge(g2->edges, edgeIndex, v1, v2, g1->edges[e].label,
                      g1->edges[e].directed, g1->edges[e].spansIncrement);
            AddEdgeToVertices(g2, edgeIndex);
            edgeIndex++;
        }
   }
}


//---------------------------------------------------------------------------
// NAME: CompressWithPredefinedSubs
//
// INPUTS: (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Searches for predefined substructures in the positive and
// negative (if given) graphs.  If found, the graphs are compressed by
// the substructure.  This function de-allocates the
// parameters->preSubs array and the graphs it points to.
//---------------------------------------------------------------------------

void CompressWithPredefinedSubs(Parameters *parameters)
{
   ULONG i;
   ULONG numPosInstances;
   ULONG numNegInstances;
   InstanceList *posInstanceList;
   InstanceList *negInstanceList;
   Substructure *sub;
   LabelList *newLabelList;

   // parameters used
   Graph *posGraph  = parameters->posGraph;
   Graph *negGraph  = parameters->negGraph;
   ULONG numPreSubs = parameters->numPreSubs;
   Graph **preSubs  = parameters->preSubs;

   for (i = 0; i < numPreSubs; i++) 
   {
      posInstanceList = FindInstances(preSubs[i], posGraph, parameters);
      numPosInstances = CountInstances(posInstanceList);
      negInstanceList = NULL;
      numNegInstances = 0;
      if (negGraph != NULL) 
      {
         negInstanceList = FindInstances(preSubs[i], negGraph, parameters);
         numNegInstances = CountInstances(negInstanceList);
      }
      // if found some instances, then report and compress
      if ((numPosInstances > 0) || (numNegInstances > 0)) 
      {
         sub = AllocateSub();
         sub->definition = preSubs[i];
         printf ("Found %lu instances of predefined substructure %lu:\n",
                 (numPosInstances + numNegInstances), (i + 1));
         if (numPosInstances > 0) 
         {
            printf("  %lu instances in positive graph\n", numPosInstances);
            sub->instances = posInstanceList;
            sub->numInstances = numPosInstances;
         }
         if (numNegInstances > 0) 
         {
            printf("  %lu instances in negative graph\n", numNegInstances);
            sub->negInstances = negInstanceList;
            sub->numNegInstances = numNegInstances;
         }
         printf("  Compressing...\n");
         CompressFinalGraphs(sub, parameters, (i + 1), TRUE);
         posGraph = parameters->posGraph;
         negGraph = parameters->negGraph;
         FreeSub(sub);
      } 
      else 
      {
         // no instances, so free the graph and the empty instance lists
         FreeGraph(preSubs[i]);
         FreeInstanceList(posInstanceList);
         FreeInstanceList(negInstanceList);
      }
   }
   free(parameters->preSubs);
   parameters->preSubs = NULL;

   // compress label list and recompute graphs' labels
   newLabelList = AllocateLabelList();
   CompressLabelListWithGraph(newLabelList, posGraph, parameters);
   if (negGraph != NULL)
      CompressLabelListWithGraph(newLabelList, negGraph, parameters);
   FreeLabelList(parameters->labelList);
   parameters->labelList = newLabelList;
    
   // recompute graphs' MDL (necessary since label list has changed)
   if (parameters->evalMethod == EVAL_MDL)
   {
      parameters->posGraphDL =
         MDL(posGraph, newLabelList->numLabels, parameters);
      if (parameters->negGraph != NULL)
         parameters->negGraphDL =
            MDL(negGraph, newLabelList->numLabels, parameters);
   }
}
