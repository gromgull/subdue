//---------------------------------------------------------------------------
// evaluate.c
//
// Substructure evaluation functions.  The default evaluation is based
// on MDL, but a simpler, size-based evaluation is also available and
// selectable through input parameters.
//
// Subdue 5.
//---------------------------------------------------------------------------

#include "subdue.h"


//---------------------------------------------------------------------------
// NAME: EvaluateSub
//
// INPUTS: (Substructure *sub) - substructure to evaluate
//         (Parameters *parameters)
//
// RETURN: (void)
//
// PURPOSE: Set the value of the substructure.  The value of a
// substructure S in a graph G is computed as
//
//   size(G) / (size(S) + size(G|S))
//
// The value of size() depends on whether we are using the EVAL_MDL or
// EVAL_SIZE evaluation method.  If EVAL_MDL, then size(g) computes
// the description length in bits of g.  If EVAL_SIZE, then size(g) is
// simply vertices(g)+edges(g).  The size(g|s) is the size of graph g
// after compressing it with substructure s.  Compression involves
// replacing each instance of s in g with a new single vertex (e.g.,
// Sub_1) and reconnecting edges external to the instance to point to
// the new vertex.  For EVAL_SIZE, the size(g|s) can be computed
// without actually performing the compression.
//
// If negative graphs Gn are present, then the evaluation becomes
//
//                    size(Gp) + size(Gn)
//        --------------------------------------------
//        size(S) + size(Gp|S) + size(Gn) - size(Gn|S)
//
// If the evaluation method is EVAL_SETCOVER, then the evaluation of
// substructure S becomes
//
//   (num pos egs containing S) + (num neg egs not containing S)
//   -----------------------------------------------------------
//               (num pos egs) + (num neg egs)
//
//---------------------------------------------------------------------------

void EvaluateSub(Substructure *sub, Parameters *parameters)
{
   double sizeOfSub;
   double sizeOfPosGraph;
   double sizeOfNegGraph;
   double sizeOfCompressedPosGraph;
   double sizeOfCompressedNegGraph;
   double subValue = 0.0;
   double sizeOfPosIncrement;
   double sizeOfNegIncrement;
   Graph *compressedGraph;
   ULONG numLabels;
   ULONG posEgsCovered;
   ULONG negEgsCovered;
   Graph *oldDefinition = NULL;

   // parameters used
   Graph *posGraph              = parameters->posGraph;
   Graph *negGraph              = parameters->negGraph;
   double posGraphDL            = parameters->posGraphDL;
   double negGraphDL            = parameters->negGraphDL;
   ULONG numPosEgs              = parameters->numPosEgs;
   ULONG numNegEgs              = parameters->numNegEgs;
   LabelList *labelList         = parameters->labelList;
   BOOLEAN allowInstanceOverlap = parameters->allowInstanceOverlap;
   ULONG evalMethod             = parameters->evalMethod;

   // calculate number of examples covered by this substructure
   sub->numExamples = PosExamplesCovered(sub, parameters);
   sub->numNegExamples = 0;
   if (negGraph != NULL) 
   {
      sub->numNegExamples = NegExamplesCovered(sub, parameters);
   }

   // if recursive sub, then temporarily add recursive edge to sub's definition
   if (sub->recursive) 
   {
      oldDefinition = sub->definition;
      sub->definition = CopyGraph(oldDefinition);
      // recursive edge added as a directed self edge (***** hack?)
      AddEdge(sub->definition, 0, 0, TRUE, sub->recursiveEdgeLabel, FALSE);
   }

   switch(evalMethod) 
   {
      case EVAL_MDL:
         numLabels = labelList->numLabels;
         sizeOfSub = MDL(sub->definition, numLabels, parameters);
         sizeOfPosGraph = posGraphDL; // cached at beginning
         compressedGraph = CompressGraph(posGraph, sub->instances, parameters);
         numLabels++; // add one for new "SUB" vertex label
         if ((allowInstanceOverlap) &&
             ((InstancesOverlap(sub->instances)) ||
              (InstancesOverlap(sub->negInstances))))
            numLabels++; // add one for new "OVERLAP" edge label
         sizeOfCompressedPosGraph = MDL(compressedGraph, numLabels, parameters);
         // add extra bits to describe where external edges connect to instances
         sizeOfCompressedPosGraph +=
            ExternalEdgeBits(compressedGraph,sub->definition,sub->numInstances);
         subValue = sizeOfPosGraph / (sizeOfSub + sizeOfCompressedPosGraph);
         FreeGraph(compressedGraph);
         if (negGraph != NULL) 
         {
            sizeOfNegGraph = negGraphDL; // cached at beginning
            compressedGraph = CompressGraph(negGraph, sub->negInstances,
                                            parameters);
            sizeOfCompressedNegGraph = MDL(compressedGraph,numLabels,
                                           parameters);
            // add extra bits to describe where external edges connect to 
            // instances
            sizeOfCompressedNegGraph +=
               ExternalEdgeBits(compressedGraph, sub->definition,
                                sub->numNegInstances);
            subValue = (sizeOfPosGraph + sizeOfNegGraph) /
                       (sizeOfSub + sizeOfCompressedPosGraph + sizeOfNegGraph -
                        sizeOfCompressedNegGraph);
            FreeGraph(compressedGraph);
         }
      break;

      case EVAL_SIZE:
         sizeOfSub = (double) GraphSize(sub->definition);
         if (parameters->incremental)
         {
            sizeOfPosIncrement = IncrementSize(parameters,
                                    GetCurrentIncrementNum(parameters), POS);
            sub->posIncrementValue = (double) SizeOfCompressedGraph(posGraph,
                                               sub->instances, parameters, POS);
            subValue = sizeOfPosIncrement /
                       (sizeOfSub + sub->posIncrementValue);
            if (negGraph != NULL)
            {
               sizeOfNegIncrement = IncrementSize(parameters,
                                       GetCurrentIncrementNum(parameters), NEG);
               sub->negIncrementValue = (double) SizeOfCompressedGraph(negGraph,
                                            sub->negInstances, parameters, NEG);
               subValue = (sizeOfPosIncrement + sizeOfNegIncrement) /
                  (sizeOfSub + sub->posIncrementValue +
                   sizeOfNegIncrement - sub->negIncrementValue);
            }
         }
         else
         {
            sizeOfPosGraph = (double) GraphSize(posGraph);
            sizeOfCompressedPosGraph =
              (double) SizeOfCompressedGraph(posGraph, sub->instances,
                                             parameters, POS); // iSubdue
            subValue = sizeOfPosGraph / (sizeOfSub + sizeOfCompressedPosGraph);
            if (negGraph != NULL)
            {
               sizeOfNegGraph = (double) GraphSize (negGraph);
               sizeOfCompressedNegGraph =
                  (double) SizeOfCompressedGraph(negGraph, sub->negInstances,
                                                  parameters, NEG);
               subValue = (sizeOfPosGraph + sizeOfNegGraph) /
                  (sizeOfSub + sizeOfCompressedPosGraph + sizeOfNegGraph -
                   sizeOfCompressedNegGraph);
            }
         }

      break;

      case EVAL_SETCOVER:
         if (parameters->incremental)
         {
            numPosEgs = IncrementNumExamples(parameters,
                           GetCurrentIncrementNum(parameters), POS);
            numNegEgs = IncrementNumExamples(parameters,
                           GetCurrentIncrementNum(parameters), NEG);
         }
         posEgsCovered = PosExamplesCovered(sub, parameters);
         if (parameters->incremental)
            sub->posIncrementValue = posEgsCovered;
         negEgsCovered = 0;
         if (negGraph != NULL)
         {
            negEgsCovered = NegExamplesCovered(sub, parameters);
            if (parameters->incremental)
               sub->negIncrementValue = negEgsCovered;
         }
         subValue = ((double) (posEgsCovered + (numNegEgs - negEgsCovered))) /
                    ((double) (numPosEgs + numNegEgs));
         break;
   }

   sub->value = subValue;

   // if recursive sub, then restore old definition
   if (sub->recursive) 
   {
      FreeGraph(sub->definition);
      sub->definition = oldDefinition;
   }
}


//---------------------------------------------------------------------------
// NAME: GraphSize
//
// INPUTS: (Graph *graph)
//
// RETURN: (ULONG) - size of graph
//
// PURPOSE: Return size of graph as vertices plus edges.
//---------------------------------------------------------------------------

ULONG GraphSize(Graph *graph)
{
   return(graph->numVertices + graph->numEdges);
}


//---------------------------------------------------------------------------
// NAME: MDL
//
// INPUTS: (Graph *graph) - graph whose description length to be computed
//         (ULONG numLabels) - number of labels from which to choose for
//                             the labels in graph
//         (Parameters *parameters)
//
// RETURN: (double) - description length of graph in bits
//
// PURPOSE: Computes a minimal encoding of the graph in terms of bits
// as follows: 
//
//   MDL (graph) = vertexBits + rowBits + edgeBits
//
//   vertexBits = lg(V) + V * lg(L)
//
//   rowBits = (V+1) * lg(B+1) + sum(i=0,V) lg C(V,k_i)
//
//   edgeBits = E * (1 + lg(L)) + (K+1) * lg(M)
//
// V is the number of vertices in the graph, E is the number of edges
// in the graph, and L is the number of unique labels (numLabels) in
// the graph.  If we assume an adjacency matrix representation of the
// graph, such that A[i,j] = 1 if there are one or more edges between
// vertex i and j, then k_i is the number of 1s in the ith row of A,
// then B = max(k_i) and K = sum(i=0,V) k_i.  Finally, M is the
// maximum number of edges between any two vertices in the graph.
// 
// While the encoding is not provably minimal, a lot of work has been
// done to make it minimal.  See the Subdue papers for details on the
// encoding.
//---------------------------------------------------------------------------

double MDL(Graph *graph, ULONG numLabels, Parameters *parameters)
{
   double vertexBits;
   double rowBits;
   double edgeBits;
   double totalBits;
   ULONG v1;
   ULONG V;  // number of vertices
   ULONG E;  // number of edges
   ULONG L;  // number of unique labels
   ULONG ki; // number of 1s in row i of adjacency matrix
   ULONG B;  // maximum number of 1s in a row of the adjacency matrix
   ULONG K;  // number of 1s in adjacency matrix
   ULONG M;  // maximum number of edges between any two vertices
   ULONG tmpM;

   V = graph->numVertices;
   E = graph->numEdges;
   L = numLabels;
   vertexBits = Log2(V) + (V * Log2(L));
   rowBits = V * Log2Factorial(V, parameters);
   edgeBits = E * (1 + Log2(L));
   B = 0;
   K = 0;
   M = 0;
   for (v1 = 0; v1 < V; v1++) 
   {
      ki = NumUniqueEdges(graph, v1);
      rowBits -= (Log2Factorial(ki, parameters) +
                  Log2Factorial((V - ki), parameters));
      if (ki > B) 
      {
         B = ki;
      }
      K += ki;
      tmpM = MaxEdgesToSingleVertex(graph, v1);
      if (tmpM > M) 
      {
         M = tmpM;
      }
   }
   rowBits += ((V + 1) * Log2(B + 1));
   edgeBits += ((K + 1) * Log2(M));
   totalBits = vertexBits + rowBits + edgeBits;

   return totalBits;
}


//---------------------------------------------------------------------------
// NAME: NumUniqueEdges
//
// INPUTS: (Graph *graph) - graph containing vertex
//         (ULONG v1) - vertex to find number of unique edges
//
// RETURN: (ULONG) - number of unique edges of vertex v1
//
// PURPOSE: Compute the number of different vertices that vertex v1 has an
// edge to.  If edge is undirected, then it is included only if connected
// to a larger-numbered vertex (or itself).  This prevents double counting
// undirected edges.
//---------------------------------------------------------------------------

ULONG NumUniqueEdges(Graph *graph, ULONG v1)
{
   ULONG e;
   ULONG v2;
   Edge *edge;
   ULONG numUniqueEdges;

   numUniqueEdges = 0;
   // look through all edges of vertex v1
   for (e = 0; e < graph->vertices[v1].numEdges; e++) 
   {
      edge = & graph->edges[graph->vertices[v1].edges[e]];
      if (edge->vertex1 == v1)
         v2 = edge->vertex2;
      else 
         v2 = edge->vertex1;
      // check if edge connected to unique vertex
      if (((edge->directed) && (edge->vertex1 == v1)) || // out-going edge
          ((! edge->directed) && (v2 >= v1))) 
      {
         if (! graph->vertices[v2].used) 
         {
            numUniqueEdges++;
            graph->vertices[v2].used = TRUE;
         }
      }
   }
   // reset vertex used flags
   for (e = 0; e < graph->vertices[v1].numEdges; e++) 
   {
      edge = & graph->edges[graph->vertices[v1].edges[e]];
      graph->vertices[edge->vertex1].used = FALSE;
      graph->vertices[edge->vertex2].used = FALSE;
   }
   return numUniqueEdges;
}


//---------------------------------------------------------------------------
// NAME: MaxEdgesToSingleVertex
//
// INPUTS: (Graph *graph) - graph containing vertex
//         (ULONG v1) - vertex to look for maximum edges
//
// RETURN: (ULONG) - maximum number of edges from v1 to another vertex
//
// PURPOSE: Computes the maximum number of edges between v1 and
// another vertex (including v1 itself).
//---------------------------------------------------------------------------

ULONG MaxEdgesToSingleVertex(Graph *graph, ULONG v1)
{
   ULONG maxEdges;
   ULONG numEdges;
   ULONG i, j;
   ULONG v2, v2j;
   Edge *edge1, *edge2;
   ULONG numEdgesToVertex2;

   maxEdges = 0;
   numEdges = graph->vertices[v1].numEdges;
   for (i = 0; i < numEdges; i++) 
   {
      edge1 = & graph->edges[graph->vertices[v1].edges[i]];
      // get other vertex
      if (edge1->vertex1 == v1)
         v2 = edge1->vertex2;
      else 
         v2 = edge1->vertex1;
      if (((edge1->directed) && (edge1->vertex1 == v1)) || // outgoing edge
          ((! edge1->directed) && (v2 >= v1))) 
      {
         // count how many edges to v2
         numEdgesToVertex2 = 1;
         for (j = i + 1; j < numEdges; j++) 
         { //start at i+1 since only want max
            edge2 = & graph->edges[graph->vertices[v1].edges[j]];
            if (edge2->vertex1 == v1)
               v2j = edge2->vertex2;
            else 
               v2j = edge2->vertex1;
            if (v2j == v2) 
            {
               // outgoing edge
               if (((edge2->directed) && (edge2->vertex1 == v1)) ||
                   ((! edge1->directed) && (v2j >= v1))) 
               {
                  numEdgesToVertex2++;
               }
            }
         }
         if (numEdgesToVertex2 > maxEdges)
            maxEdges = numEdgesToVertex2;
      }
   }
   return maxEdges;
}


//---------------------------------------------------------------------------
// NAME: ExternalEdgeBits
//
// INPUTS: (Graph *compressedGraph) - graph compressed with a substructure
//         (Graph *subGraph) - substructure's graph definition
//         (ULONG numInstances) - number of substructure instances
//
// RETURN: (double) - bits describing external edge connections
//
// PURPOSE: Computes and returns the number of bits necessary to
// describe which vertex in a substructure's instance does an external
// edge connect to.  This is computed as lg(vertices(S)), where S is
// the substructure used to compress the graph.  While this
// information is not retained in the compressed graph, it is
// important to include in the MDL computation so that the values are
// consistent with a quasi-lossless compression.
//
// Note that this procedure assumes that the "SUB" vertices in
// compressedGraph occupy the first numInstances vertices in the
// graph's vertex array.  This property is maintained by the
// CompressGraph() procedure.
//---------------------------------------------------------------------------

double ExternalEdgeBits(Graph *compressedGraph, Graph *subGraph,
                        ULONG numInstances)
{
   ULONG v;
   ULONG e;
   Vertex *vertex;
   Edge *edge;
   double log2SubVertices;
   double edgeBits;

   log2SubVertices = Log2(subGraph->numVertices);
   edgeBits = 0.0;
   // add bits for each edge connected to "SUB" vertex
   for (v = 0; v < numInstances; v++) 
   {
      vertex = & compressedGraph->vertices[v];
      for (e = 0; e < vertex->numEdges; e++) 
      {
         edge = & compressedGraph->edges[vertex->edges[e]];
         edgeBits += log2SubVertices;
         if (edge->vertex1 == edge->vertex2) // self-edge
            edgeBits += log2SubVertices;
      }
   }
   return edgeBits;
}

//---------------------------------------------------------------------------
// NAME: Log2Factorial
//
// INPUTS: (ULONG number)
//         (Parameters *parameters)
//
// RETURN: (double) - lg (number !)
//
// PURPOSE: Computes the log base 2 of the factorial of the given
// number.  Since this is done often, a cache is maintained in
// parameters->log2Factorial.  NOTE: this procedure assumes that
// parameters->log2Factorial[0..1] already set before first call.
//---------------------------------------------------------------------------

double Log2Factorial(ULONG number, Parameters *parameters)
{
   ULONG i;
   ULONG newSize;

   if (number >= parameters->log2FactorialSize) 
   {
      // add enough room to array to encompass desired value and then some
      newSize = number + LIST_SIZE_INC;
      parameters->log2Factorial = (double *)
         realloc(parameters->log2Factorial, newSize * sizeof(double));
      if (parameters->log2Factorial == NULL)
         OutOfMemoryError("Log2Factorial");
      // compute new values
      for (i = parameters->log2FactorialSize; i < newSize; i++) 
      {
         parameters->log2Factorial[i] =
            Log2(i) + parameters->log2Factorial[i - 1];
      }
      parameters->log2FactorialSize = newSize;
   }
   return parameters->log2Factorial[number];
}


//---------------------------------------------------------------------------
// NAME: Log2
//
// INPUTS: (ULONG number)
//
// RETURN: (double)
//
// PURPOSE: Computes the log base 2 of the given number.
//---------------------------------------------------------------------------

double Log2(ULONG number)
{
   if (number <= 0)
      return 0.0;
   else
      return log((double) number) / LOG_2;
}


//---------------------------------------------------------------------------
// NAME: PosExamplesCovered
//
// INPUTS: (Substructure *sub) - substructure to look for in pos examples
//         (Parameters *parameters)
//
// RETURN: (ULONG) - number of positive examples covered by sub
//
// PURPOSE: Returns the number of positive examples that contain the
// substructure sub as a subgraph.
//---------------------------------------------------------------------------

ULONG PosExamplesCovered(Substructure *sub, Parameters *parameters)
{
   ULONG start = 0;
   Increment *increment = NULL;

   if (parameters->incremental)
   {
      increment = GetCurrentIncrement(parameters);
      start = increment->startPosVertexIndex;
   }

   return ExamplesCovered(sub->instances,
                          parameters->posGraph,
                          parameters->numPosEgs,
                          parameters->posEgsVertexIndices, start);
}


//---------------------------------------------------------------------------
// NAME: NegExamplesCovered
//
// INPUTS: (Substructure *sub) - substructure to look for in neg examples
//         (Parameters *parameters)
//
// RETURN: (ULONG) - number of negative examples covered by sub
//
// PURPOSE: Returns the number of negative examples that contain the
// substructure sub as a subgraph.
//---------------------------------------------------------------------------

ULONG NegExamplesCovered(Substructure *sub, Parameters *parameters)
{
   ULONG start = 0;
   Increment *increment = NULL;

   if (parameters->incremental)
   {
      increment = GetCurrentIncrement(parameters);
      start = increment->startNegVertexIndex;
   }

   return ExamplesCovered(sub->negInstances,
                          parameters->negGraph,
                          parameters->numNegEgs,
                          parameters->negEgsVertexIndices, start);
}


//---------------------------------------------------------------------------
// NAME: ExamplesCovered
//
// INPUTS: (InstanceList *instanceList) - instances of substructure
//         (Graph *graph) - graph containing instances and examples
//         (ULONG numEgs) - number of examples to consider
//         (ULONG *egsVertexIndices) - vertex indices of each examples
//           starting vertex
//         (ULONG start) - start vertex for current increment
//
// RETURN: (ULONG) - number of examples covered by instances
//
// PURPOSE: Return the number of examples, whose starting vertices are
// stored in egsVertexIndices, are covered by an instance in
// instanceList.  Note that one example may contain more than one
// instance.
//---------------------------------------------------------------------------

ULONG ExamplesCovered(InstanceList *instanceList, Graph *graph,
                      ULONG numEgs, ULONG *egsVertexIndices, ULONG start)
{
   ULONG i;
   ULONG egStartVertexIndex;
   ULONG egEndVertexIndex;
   ULONG instanceVertexIndex;
   InstanceListNode *instanceListNode;
   BOOLEAN found;
   ULONG numEgsCovered;

   numEgsCovered = 0;
   if (instanceList != NULL) 
   {
      // for each example, look for a covering instance
      for (i = 0; i < numEgs; i++)
      {
         // find example's starting and ending vertex indices
         egStartVertexIndex = egsVertexIndices[i];
         if (egStartVertexIndex >= start)
         {
            if (i < (numEgs - 1))
               egEndVertexIndex = egsVertexIndices[i + 1] - 1;
            else
               egEndVertexIndex = graph->numVertices - 1;
            // look for an instance whose vertices are in range
            instanceListNode = instanceList->head;
            found = FALSE;
            while ((instanceListNode != NULL) && (! found))
            {
               // can check any instance vertex, so use the first
               instanceVertexIndex = instanceListNode->instance->vertices[0];
               if ((instanceVertexIndex >= egStartVertexIndex) &&
                   (instanceVertexIndex <= egEndVertexIndex))
               {
                  // found an instance covering this example
                  numEgsCovered++;
                  found = TRUE;
               }
               instanceListNode = instanceListNode->next;
            }
         }
      }
   }
   return numEgsCovered;
}
