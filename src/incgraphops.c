//---------------------------------------------------------------------------
// incgraphops.c
//
// Functions for managing the reference graphs used in the discovery of
// sub instances that span increment boundaries.
//
// Much of this code was derived from the graphops code.
//
// Subdue 5
//---------------------------------------------------------------------------

#include "subdue.h"


//---------------------------------------------------------------------------
// NAME: MarkGraphEdgesUsed
//
// INPUT: (ReferenceGraph *refGraph) - reference graph
//        (Graph *fullGraph) - complete input graph
//        (BOOLEAN value) - true or false value for the edge
//
// RETURN:  void
//
// PURPOSE:  Find each edge in the full graph that is present in the ref graph,
//           mark the "used" flag as true or false.
//---------------------------------------------------------------------------

void MarkGraphEdgesUsed(ReferenceGraph *refGraph, Graph *fullGraph,
                        BOOLEAN value)
{
   ULONG e;

   for (e = 0; e < refGraph->numEdges; e++)
      fullGraph->edges[refGraph->edges[e].map].used = value;
}


//---------------------------------------------------------------------------
// NAME: MarkGraphEdgesValid
//
// INPUT: (ReferenceGraph *refGraph) - the reference graph
//        (Graph *fullGraph) - the complete graph
//        (BOOLEAN mark) - if true then mark the full graph;
//                         if false then reset the full graph to true.
//
// RETURN:  void
//
// PURPOSE:  Mark the edges in fullGraph as valid/invalid based on their state
//           in the ref graph.  If the BOOLEAN parm "mark" is true then we are
//           marking the fullGraph edges based on the refGraph edges.
//           If false, then we are resetting the fullGraph edges to TRUE.
//---------------------------------------------------------------------------

void MarkGraphEdgesValid(ReferenceGraph *refGraph, Graph *fullGraph,
                         BOOLEAN mark)
{
   ULONG e;

   for (e = 0; e < refGraph->numEdges; e++)
   {
      if (mark)
      {
         if (refGraph->edges[e].failed)
            fullGraph->edges[refGraph->edges[e].map].validPath = FALSE;
         else
            fullGraph->edges[refGraph->edges[e].map].validPath = TRUE;
      }
      else // unmark
         fullGraph->edges[refGraph->edges[e].map].validPath = TRUE;
   }
}


//---------------------------------------------------------------------------
// NAME: MarkRefGraphInstanceEdges
//
// INPUT: (Instance *instance) - the instance to be used for marking
//        (ReferenceGraph *refGraph) - the graph to mark
//        (BOOLEAN value) - the value to mark
//
// RETURN:  void
//
// PURPOSE:  Mark the edges in the ref graph corresponding to the edges in the
//           instance as used (true/false).
//---------------------------------------------------------------------------

void MarkRefGraphInstanceEdges(Instance *instance, ReferenceGraph *refGraph,
                               BOOLEAN value)
{
   ULONG e;

   for (e = 0; e < instance->numEdges; e++)
      refGraph->edges[instance->edges[e]].used = value;
}


//---------------------------------------------------------------------------
// NAME:    CopyReferenceGraph
//
// INPUTS:  (Graph *g) - graph to be copied
//
// RETURN:  (ReferenceGraph *) - pointer to copy of graph
//
// PURPOSE:  Create and return a copy of the given ref graph.
//---------------------------------------------------------------------------

ReferenceGraph *CopyReferenceGraph(ReferenceGraph *g)
{
   ReferenceGraph *gCopy;
   ULONG nv;
   ULONG ne;
   ULONG v;
   ULONG e;
   ULONG numEdges;

   nv = g->numVertices;
   ne = g->numEdges;

   // allocate graph
   gCopy = AllocateReferenceGraph(nv, ne);

   // copy vertices; allocate and copy vertex edge arrays
   for (v = 0; v < nv; v++)
   {
      gCopy->vertices[v].label = g->vertices[v].label;
      gCopy->vertices[v].map = g->vertices[v].map;
      gCopy->vertices[v].used = g->vertices[v].used;
      gCopy->vertices[v].vertexValid = g->vertices[v].vertexValid;
      numEdges = g->vertices[v].numEdges;
      gCopy->vertices[v].numEdges = numEdges;
      gCopy->vertices[v].edges = NULL;
      if (numEdges > 0)
      {
         gCopy->vertices[v].edges =
            (ULONG *) malloc (numEdges * sizeof (ULONG));
         if (gCopy->vertices[v].edges == NULL)
            OutOfMemoryError ("CopyGraph:edges");
         for (e = 0; e < numEdges; e++)
            gCopy->vertices[v].edges[e] = g->vertices[v].edges[e];
      }
   }

   // copy edges
   for (e = 0; e < ne; e++)
   {
      gCopy->edges[e].vertex1 = g->edges[e].vertex1;
      gCopy->edges[e].vertex2 = g->edges[e].vertex2;
      gCopy->edges[e].label = g->edges[e].label;
      gCopy->edges[e].directed = g->edges[e].directed;
      gCopy->edges[e].used = g->edges[e].used;
      gCopy->edges[e].map = g->edges[e].map;
      gCopy->edges[e].failed = g->edges[e].failed;
   }
   return gCopy;
}


//---------------------------------------------------------------------------
// NAME: AddReferenceVertex
//
// INPUT: (ReferenceGraph *graph) - ref graph
//        (ULONG labelIndex) - the vertex's label index
//
// RETURN:  void
//
// PURPOSE:  Adds a vertex to a ref graph.
//---------------------------------------------------------------------------

void AddReferenceVertex (ReferenceGraph *graph, ULONG labelIndex)
{
   ReferenceVertex *newVertexList;
   ULONG numVertices = graph->numVertices;
   ULONG vertexListSize = graph->vertexListSize;

   // make sure there is enough room for another vertex
   if (vertexListSize == graph->numVertices)
   {
      vertexListSize += LIST_SIZE_INC;
      newVertexList = (ReferenceVertex *) realloc
        (graph->vertices, (sizeof (ReferenceVertex) * vertexListSize));
      if (newVertexList == NULL)
         OutOfMemoryError("vertex list");
      graph->vertices = newVertexList;
      graph->vertexListSize = vertexListSize;
   }

   // store information in vertex
   graph->vertices[numVertices].label = labelIndex;
   graph->vertices[numVertices].numEdges = 0;
   graph->vertices[numVertices].edges = NULL;
   graph->vertices[numVertices].map = VERTEX_UNMAPPED;
   graph->vertices[numVertices].used = FALSE;
   graph->vertices[numVertices].vertexValid = TRUE;
   graph->numVertices++;
}


//------------------------------------------------------------------------------
// NAME: AddReferenceEdge
//
// INPUT: (ReferenceGraph *graph) - ref graph
//        (ULONG sourceVertexIndex) - source of the edge
//        (ULONG targetVertexIndex) - target of the edge
//        (BOOLEAN directed)
//        (ULONG labelIndex) - edge's label index
//        (BOOLEAN spansIncrement) - true if edge spans the increment boundary
//
// RETURN:  void
//
// PURPOSE:  Add an edge to a ref graph.
//------------------------------------------------------------------------------

void AddReferenceEdge(ReferenceGraph *graph, ULONG sourceVertexIndex,
                      ULONG targetVertexIndex, BOOLEAN directed,
                      ULONG labelIndex, BOOLEAN spansIncrement)
{
   ReferenceEdge *newEdgeList;
   ULONG edgeListSize = graph->edgeListSize;

   // make sure there is enough room for another edge in the graph
   if (edgeListSize == graph->numEdges)
   {
      edgeListSize += LIST_SIZE_INC;
      newEdgeList =
         (ReferenceEdge *) realloc(graph->edges, (sizeof (ReferenceEdge) * edgeListSize));
      if (newEdgeList == NULL)
         OutOfMemoryError("AddEdge:newEdgeList");
      graph->edges = newEdgeList;
      graph->edgeListSize = edgeListSize;
   }

   // add edge to graph
   graph->edges[graph->numEdges].vertex1 = sourceVertexIndex;
   graph->edges[graph->numEdges].vertex2 = targetVertexIndex;
   graph->edges[graph->numEdges].spansIncrement = spansIncrement;
   graph->edges[graph->numEdges].label = labelIndex;
   graph->edges[graph->numEdges].directed = directed;
   graph->edges[graph->numEdges].used = FALSE;
   graph->edges[graph->numEdges].failed = FALSE;

   // add index to edge in edge index array of both vertices
   AddRefEdgeToRefVertices(graph, graph->numEdges);

   graph->numEdges++;
}


//-----------------------------------------------------------------------------
// NAME: FreeReferenceGraph
//
// INPUT:  (ReferenceGraph *graph) - the ref graph
//
// RETURN:  void
//
// PURPOSE:  Free the ref graph.
//-----------------------------------------------------------------------------

void FreeReferenceGraph (ReferenceGraph *graph)
{
   ULONG v;

   if (graph != NULL)
   {
      for (v = 0; v < graph->numVertices; v++)
         free (graph->vertices[v].edges);
      free (graph->edges);
      free (graph->vertices);
      free (graph);
   }
}


//-----------------------------------------------------------------------------
// NAME: AllocateReferenceGraph
//
// INPUT: (ULONG v) - number of vertices
//        (ULONG e) - number of edges
//
// RETURN: (ReferenceGraph *) -the new ref graph
//
// PURPOSE:  Create a new reference graph.
//-----------------------------------------------------------------------------

ReferenceGraph *AllocateReferenceGraph (ULONG v, ULONG e)
{
   ReferenceGraph *graph;
   graph = (ReferenceGraph *) malloc (sizeof (ReferenceGraph));

   if (graph == NULL)
      OutOfMemoryError("AllocateReferenceGraph:graph");
   graph->numVertices = v;
   graph->numEdges = e;
   graph->vertices = NULL;
   graph->edges = NULL;
   if (v > 0)
   {
      graph->vertices =
         (ReferenceVertex *) malloc (sizeof (ReferenceVertex) * v);
      if (graph->vertices == NULL)
         OutOfMemoryError ("AllocateReferenceGraph:graph->vertices");
   }
   graph->vertexListSize = v;
   if (e > 0)
   {
      graph->edges = (ReferenceEdge *) malloc (sizeof (ReferenceEdge) * e);
      if (graph->edges == NULL)
         OutOfMemoryError ("AllocateReferenceGraph:graph->edges");
   }
   graph->edgeListSize = e;
   return graph;
}


//-----------------------------------------------------------------------------
// NAME: AddRefEdgeToRefVertices
//
// INPUT: (ReferenceGraph *graph) - ref graph
//        (ULONG edgeIndex) - the edge
//
// RETURN:  void
//
// PURPOSE:  For both vertices connected to an edge we add the edge to the
//           vertices' edge list.
//-----------------------------------------------------------------------------

void AddRefEdgeToRefVertices(ReferenceGraph *graph, ULONG edgeIndex)
{
   ULONG v1, v2;
   ReferenceVertex *vertex;
   ULONG *edgeIndices;

   v1 = graph->edges[edgeIndex].vertex1;
   v2 = graph->edges[edgeIndex].vertex2;
   vertex = & graph->vertices[v1];
   edgeIndices =
      (ULONG *) realloc(vertex->edges, sizeof (ULONG) * (vertex->numEdges + 1));
   if (edgeIndices == NULL)
      OutOfMemoryError ("AddRefEdgeToRefVertices:edgeIndices1");
   edgeIndices[vertex->numEdges] = edgeIndex;
   vertex->edges = edgeIndices;
   vertex->vertexValid = TRUE;
   vertex->numEdges++;

   if (v1 != v2)                        // do not add a self edge twice
   {
      vertex = & graph->vertices[v2];
      edgeIndices = (ULONG *) realloc (vertex->edges,
                                       sizeof (ULONG) * (vertex->numEdges + 1));
      if (edgeIndices == NULL)
         OutOfMemoryError ("AddRefEdgeToRefVertices:edgeIndices2");
      edgeIndices[vertex->numEdges] = edgeIndex;
      vertex->edges = edgeIndices;
      vertex->vertexValid = TRUE;
      vertex->numEdges++;
   }
}


//-----------------------------------------------------------------------------
// NAME: InstanceToRefGraph
//
// INPUT: (Instance *instance) - the seed instance
//        (Graph *graph) - the complete graph
//        (Parameters *parameters) - system parms
//
// RETURN:  (ReferenceGraph *) - the new ref graph
//
// PURPOSE:  Convert an instance to a ref graph.  We use this to convert the
//           initial seed instances (2 vertices spanning the increment boundary)
//           to the start of our ref graph.  Remaps the instance to the new
//           ref graph.
//-----------------------------------------------------------------------------

ReferenceGraph *InstanceToRefGraph(Instance *instance, Graph *graph,
                                   Parameters *parameters)
{
   ReferenceGraph *newGraph;
   Vertex *vertex;
   Edge *edge;
   ULONG i, j;
   ULONG v1, v2;
   BOOLEAN found1;
   BOOLEAN found2;
   ReferenceVertex *refVertex;

   v1 = 0;
   v2 = 0;
   newGraph = AllocateReferenceGraph(instance->numVertices, instance->numEdges);
 
   // convert vertices
   for (i = 0; i < instance->numVertices; i++)
   {
      vertex = & graph->vertices[instance->vertices[i]];
      newGraph->vertices[i].label = vertex->label;
      newGraph->vertices[i].numEdges = 0;
      newGraph->vertices[i].edges = NULL;
      newGraph->vertices[i].used = FALSE;
      newGraph->vertices[i].map = instance->vertices[i];
      newGraph->vertices[i].vertexValid = TRUE;
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
      newGraph->edges[i].map = instance->edges[i];
      newGraph->edges[i].label = edge->label;
      newGraph->edges[i].directed = edge->directed;
      newGraph->edges[i].used = FALSE;
      newGraph->edges[i].failed = FALSE;

      // add edge to appropriate vertices
      refVertex = & newGraph->vertices[v1];
      refVertex->vertexValid = TRUE; //this should be unnecessary
      refVertex->numEdges++;
      refVertex->edges =
         (ULONG *) realloc(refVertex->edges,
                    sizeof (ULONG) * refVertex->numEdges);
      if (refVertex->edges == NULL)
         OutOfMemoryError ("InstanceToGraph:refVertex1->edges");
      refVertex->edges[refVertex->numEdges - 1] = i;
      if (v1 != v2)
      {
         refVertex = &newGraph->vertices[v2];
         refVertex->vertexValid = TRUE;  // this should be unnecessary
         refVertex->numEdges++;
         refVertex->edges =
            (ULONG *) realloc(refVertex->edges,
                              sizeof (ULONG) * refVertex->numEdges);
         if (refVertex->edges == NULL)
            OutOfMemoryError ("InstanceToGraph:refVertex2->edges");
         refVertex->edges[refVertex->numEdges - 1] = i;
      }
   }

    // remap instance to refGraph
   for (i = 0; i < instance->numVertices; i++)
      instance->vertices[i] = i;

   for (i = 0; i < instance->numEdges; i++)
      instance->edges[i] = i;
   instance->newVertex = 0;
   instance->newEdge = 0;

   return newGraph;
}


//------------------------------------------------------------------------------
// NAME: CreateGraphRefInstance
//
// INPUT: (Instance *instance1) - instance referencing the ref graph
//        (ReferenceGraph *refGraph) - ref graph being referenced by instance
//
// RETURN:  (Instance *) - an instance referencing the full graph
//
// PURPOSE:  Take an instance that is referencing a ref graph and create an
//           instance that references the full graph.
//------------------------------------------------------------------------------

Instance *CreateGraphRefInstance(Instance *instance1, ReferenceGraph *refGraph) 
{
   int i;
   Instance *instance2 =
      AllocateInstance(instance1->numVertices, instance1->numEdges);

   for(i=0;i<instance1->numVertices;i++)
   {
      // assign the vertices to the full graph
      instance2->vertices[i] = refGraph->vertices[instance1->vertices[i]].map;
   }

   // reorder indices based on the vertices in the full graph
   SortIndices(instance2->vertices, 0, instance2->numVertices-1);

   for(i=0;i<instance1->numEdges;i++)
   {
      // assign the vertices to the full graph
      instance2->edges[i] = refGraph->edges[instance1->edges[i]].map;
   }

   // reorder indices based on the vertices in the full graph
   SortIndices(instance2->edges, 0, instance2->numEdges-1);

   return instance2;
}

//------------------------------------------------------------------------------
// NAME: SortIndices
//
// INPUT: (ULONG *A) - array being sorted
//        (long p) - start index
//        (long r) - end index
//
// RETURN:  void
//
// PURPOSE:  Implementation of the QuickSort algorithm.
//------------------------------------------------------------------------------

void SortIndices(ULONG *A, long p, long r) 
{
   long q;

   if (p < r)
   {
      q = Partition(A, p, r);
      SortIndices(A, p, q-1);
      SortIndices(A, q+1, r);		
   }
}


//------------------------------------------------------------------------------
// NAME: Partition
//
// INPUT: (ULONG *A) - input array
//        (long p) - start index
//        (ulong r) - end index
//
// RETURN:  long
//
// PURPOSE:  Partition algorithm for QuickSort.
//------------------------------------------------------------------------------

long Partition(ULONG *A, long p, long r) 
{
   ULONG x;
   long i,j;
   ULONG temp;

   x = A[r];
   i = p-1;
   for(j=p; j<=(r-1); j++) 
   {
      if(A[j] <= x) 
      {
         i++;
         // swap A[i], A[j]
         temp = A[i];
         A[i] = A[j];
         A[j] = temp;
      }
   }
   temp = A[i+1];
   A[i+1] = A[r];
   A[r] = temp;

   return (i+1);
}


//------------------------------------------------------------------------------
// NAME: AllocateRefInstanceListNode
//
// RETURN:  (RefInstanceListNode *)
//
// PURPOSE:  Create a RefInstanceListNode.
//------------------------------------------------------------------------------

RefInstanceListNode *AllocateRefInstanceListNode()
{
   RefInstanceListNode *refInstanceListNode = malloc(sizeof(RefInstanceListNode));

   refInstanceListNode->instanceList = AllocateInstanceList();
   refInstanceListNode->refGraph = NULL;
   refInstanceListNode->next = NULL;

   return refInstanceListNode;
}


//------------------------------------------------------------------------------
// NAME: AllocateRefInstanceList
//
// RETURN:  (RefInstanceList *)
//
// PURPOSE:  Create a RefInstanceList.
//------------------------------------------------------------------------------

RefInstanceList *AllocateRefInstanceList()
{
   RefInstanceList *refInstanceList = malloc(sizeof(RefInstanceList));

   refInstanceList->head = NULL;

   return refInstanceList;
}


//------------------------------------------------------------------------------
// NAME: FreeRefInstanceListNode
//
// INPUT:  (RefInstanceListNode *)
//
// RETURN:  void
//
// PURPOSE: Free the input node.
//------------------------------------------------------------------------------

void FreeRefInstanceListNode(RefInstanceListNode *refInstanceListNode)
{
   FreeInstanceList(refInstanceListNode->instanceList);
   FreeReferenceGraph(refInstanceListNode->refGraph);
   free(refInstanceListNode);
}


//------------------------------------------------------------------------------
// NAME: CopyInstance
//
// RETURN:  (Instance *) - new instance copy.
//
// PURPOSE:  Create a new copy of an instance.
//------------------------------------------------------------------------------

Instance *CopyInstance(Instance *instance)
{
   Instance *newInstance;
   ULONG i;

   newInstance = AllocateInstance(instance->numVertices, instance->numEdges);
   for (i = 0; i < instance->numVertices; i++)
      newInstance->vertices[i] = instance->vertices[i];
   for (i = 0; i < instance->numEdges; i++)
      newInstance->edges[i] = instance->edges[i];

   return newInstance;	
}


//------------------------------------------------------------------------------
// NAME: VertexInSubList
//
// INPUT: (SubList *subList) - list of top-n subs
//        (Vertex *vertex) - vertex from the full graph that we are comparing
//                           to the sub's vertices
//
// RETURN:  BOOLEAN - true if the vertex is consistent with one of the subs
//
// PURPOSE:  Check to see if this vertex is included in any of the subs.
//------------------------------------------------------------------------------

BOOLEAN VertexInSubList(SubList *subList, Vertex *vertex)
{
   SubListNode *subListNode;
   Substructure *sub;
   BOOLEAN found;

   if (subList != NULL)
      subListNode = subList->head;
   else
   {
      printf("Best SubList is null\n");
      exit(0);
   }

   // pass through each substructure
   while (subListNode != NULL) 
   {
      sub = subListNode->sub;

      // Look at each vertex index in the instance and see if it matches our
      // vertex, if so then return TRUE
      found = VertexInSub(sub->definition, vertex);

      if (found)
         return TRUE;
      subListNode = subListNode->next;
   }	

   return FALSE;
}


//------------------------------------------------------------------------------
// NAME: VertexInSub
//
// INPUT: (Graph *subDef) - the sub against which we are comparing the vertex
//        (Vertex *vertex) - vertex from the full graph that we are comparing
//                           to the sub's vertices
//
// RETURN:  BOOLEAN - true if the vertex is consistent with the sub
//
// PURPOSE:  This is designed to compare the vertices in a substructure's graph
//           definition to a vertex from a full graph
//------------------------------------------------------------------------------

BOOLEAN VertexInSub(Graph *subDef, Vertex *vertex)
{
   Vertex *vertices;
   ULONG i;

   vertices = subDef->vertices;

   // Look at each vertex index in the instance and see if it matches our vertex
   // if so then return TRUE
   for (i=0;i<subDef->numVertices;i++) 
   {
      // If label and degree are the same, we have a match.
      // The degree of the sub's vertex must be <= to the degree of the
      // graph's vertex.
      if ((vertices[i].label == vertex->label) &&
          (vertices[i].numEdges <= vertex->numEdges))
         return TRUE;
   }

   return FALSE;
}
