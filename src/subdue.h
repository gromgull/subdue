//---------------------------------------------------------------------------
// subdue.h
//
// Data type and prototype definitions for the SUBDUE system.
//
// SUBDUE, version 5.2
//---------------------------------------------------------------------------

#ifndef SUBDUE_H
#define SUBDUE_H

#include <limits.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "avl.h"

#define SUBDUE_VERSION "5.2.2"

// Substructure evaluation methods
#define EVAL_MDL      1
#define EVAL_SIZE     2
#define EVAL_SETCOVER 3

// Graph match search space limited to V^MATCH_SEARCH_THRESHOLD_EXPONENT
// If set to zero, then no limit
#define MATCH_SEARCH_THRESHOLD_EXPONENT 4.0

// Starting strings for input files
#define SUB_TOKEN        "S"  // new substructure
#define PREDEF_SUB_TOKEN "PS" // new predefined substructure
#define POS_EG_TOKEN     "XP" // new positive example
#define NEG_EG_TOKEN     "XN" // new negative example

// Vertex and edge labels used for graph compression
#define SUB_LABEL_STRING     "SUB"
#define OVERLAP_LABEL_STRING "OVERLAP"
#define PREDEFINED_PREFIX    "PS"

#define NEG 0
#define POS 1

// Costs of various graph match transformations
#define INSERT_VERTEX_COST             1.0 // insert vertex
#define DELETE_VERTEX_COST             1.0 // delete vertex
#define SUBSTITUTE_VERTEX_LABEL_COST   1.0 // substitute vertex label
#define INSERT_EDGE_COST               1.0 // insert edge
#define INSERT_EDGE_WITH_VERTEX_COST   1.0 // insert edge with vertex
#define DELETE_EDGE_COST               1.0 // delete edge
#define DELETE_EDGE_WITH_VERTEX_COST   1.0 // delete edge with vertex
#define SUBSTITUTE_EDGE_LABEL_COST     1.0 // substitute edge label
#define SUBSTITUTE_EDGE_DIRECTION_COST 1.0 // change directedness of edge
#define REVERSE_EDGE_DIRECTION_COST    1.0 // change direction of directed edge

// MPI message tags
#define MPI_MSG_EVAL     1
#define MPI_MSG_EVAL_SUB 2
#define MPI_MSG_BEST_SUB 3

// MPI_BUFFER_SIZE must be big enough to hold largest substructure (not
// including instances)
#define MPI_BUFFER_SIZE 16384

// Constants for graph matcher.  Special vertex mappings use the upper few
// unsigned long integers.  This assumes graphs will never have this many
// vertices, which is a pretty safe assumption.  The maximum double is used
// for initial costs.
#define MAX_UNSIGNED_LONG ULONG_MAX  // ULONG_MAX defined in limits.h
#define VERTEX_UNMAPPED   MAX_UNSIGNED_LONG
#define VERTEX_DELETED    MAX_UNSIGNED_LONG - 1
#define MAX_DOUBLE        DBL_MAX    // DBL_MAX from float.h

// Label types
#define STRING_LABEL  0
#define NUMERIC_LABEL 1

// General defines
#define LIST_SIZE_INC  100  // initial size and increment for realloc-ed lists
#define TOKEN_LEN     256  // maximum length of token from input graph file
#define FILE_NAME_LEN 512  // maximum length of file names
#define COMMENT       '%'  // comment character for input graph file
#define NUMERIC_OUTPUT_PRECISION 6
#define LOG_2 0.6931471805599452862 // log_e(2) pre-computed

#define SPACE ' '
#define TAB   '\t'
#define NEWLINE '\n'
#define DOUBLEQUOTE '\"'
#define CARRIAGERETURN '\r'

#define FALSE 0
#define TRUE  1

//---------------------------------------------------------------------------
// Type Definitions
//---------------------------------------------------------------------------

typedef unsigned char UCHAR;
typedef unsigned char BOOLEAN;
typedef unsigned long ULONG;

// Label
typedef struct 
{
   UCHAR labelType;       // one of STRING_LABEL or NUMERIC_LABEL
   union 
   {
      char *stringLabel;
      double numericLabel;
   } labelValue;
   BOOLEAN used;          // flag used to mark labels at various times
} Label;

// Label list
typedef struct 
{
   ULONG size;      // Number of label slots currently allocated in array
   ULONG numLabels; // Number of actual labels stored in list
   Label *labels;   // Array of labels
} LabelList;

// Edge
typedef struct 
{
   ULONG   vertex1;  // source vertex index into vertices array
   ULONG   vertex2;  // target vertex index into vertices array
   ULONG   label;    // index into label list of edge's label
   BOOLEAN directed; // TRUE if edge is directed
   BOOLEAN used;     // flag for marking edge used at various times
                     //   used flag assumed FALSE, so always reset when done
   BOOLEAN spansIncrement;   // TRUE if edge crosses a previous increment
   BOOLEAN validPath;
} Edge;

// Vertex
typedef struct 
{
   ULONG label;    // index into label list of vertex's label
   ULONG numEdges; // number of edges defined using this vertex
   ULONG *edges;   // indices into edge array of edges using this vertex
   ULONG map;      // used to store mapping of this vertex to corresponding
                   //   vertex in another graph
   BOOLEAN used;   // flag for marking vertex used at various times
                   //   used flag assumed FALSE, so always reset when done
} Vertex;

// Graph
typedef struct 
{
   ULONG  numVertices; // number of vertices in graph
   ULONG  numEdges;    // number of edges in graph
   Vertex *vertices;   // array of graph vertices
   Edge   *edges;      // array of graph edges
   ULONG  vertexListSize; // allocated size of vertices array
   ULONG  edgeListSize;   // allocated size of edges array
} Graph;

// VertexMap: vertex to vertex mapping for graph match search
typedef struct 
{
   ULONG v1;
   ULONG v2;
} VertexMap;

// Instance
typedef struct _instance
{
   ULONG numVertices;   // number of vertices in instance
   ULONG numEdges;      // number of edges in instance
   ULONG *vertices;     // ordered indices of instance's vertices in graph
   ULONG *edges;        // ordered indices of instance's edges in graph
   double minMatchCost; // lowest cost so far of matching this instance to
                        // a substructure
   ULONG newVertex;     // index into vertices array of newly added vertex
                        //    (0 means no new vertex added)
   ULONG newEdge;       // index into edges array of newly added edge
   ULONG refCount;      // counter of references to this instance; if zero,
                        //    then instance can be deallocated
   VertexMap *mapping;  // instance mapped to substructure definition
   ULONG mappingIndex1; // index of source vertex of latest mapping
   ULONG mappingIndex2; // index of target vertex of latest mapping
   BOOLEAN used;        // flag indicating instance already associated
                        // with a substructure (for the current iteration)
   struct _instance *parentInstance;  // pointer to parent instance
} Instance;

// InstanceListNode: node in singly-linked list of instances
typedef struct _instance_list_node 
{
   Instance *instance;
   struct _instance_list_node *next;
} InstanceListNode;

// InstanceList: singly-linked list of instances
typedef struct 
{
   InstanceListNode *head;
} InstanceList;

// Substructure
typedef struct 
{
   Graph  *definition;         // graph definition of substructure
   ULONG  numInstances;        // number of positive instances
   ULONG  numExamples;         // number of unique positive examples
   InstanceList *instances;    // instances in positive graph
   ULONG  numNegInstances;     // number of negative instances
   ULONG  numNegExamples;      // number of unique negative examples
   InstanceList *negInstances; // instances in negative graph
   double value;               // value of substructure
   BOOLEAN recursive;          // is this substructure recursive?
   ULONG recursiveEdgeLabel;   // index into label list of recursive edge label
   double posIncrementValue;   // DL/#Egs value of sub for positive increment
   double negIncrementValue;   // DL/#Egs value of sub for negative increment
} Substructure;

// SubListNode: node in singly-linked list of substructures
typedef struct _sub_list_node 
{
   Substructure *sub;
   struct _sub_list_node *next;
} SubListNode;

// SubList: singly-linked list of substructures
typedef struct 
{
   SubListNode *head;
} SubList;

// MatchHeapNode: node in heap for graph match search queue
typedef struct 
{
   ULONG  depth; // depth of node in search space (number of vertices mapped)
   double cost;  // cost of mapping
   VertexMap *mapping;
} MatchHeapNode;

// MatchHeap: heap of match nodes
typedef struct 
{
   ULONG size;      // number of nodes allocated in memory
   ULONG numNodes;  // number of nodes in heap
   MatchHeapNode *nodes;
} MatchHeap;

// ReferenceEdge
typedef struct
{
   ULONG   vertex1;  // source vertex index into vertices array
   ULONG   vertex2;  // target vertex index into vertices array
   BOOLEAN spansIncrement; //true if edge spans to or from a previous increment
   ULONG   label;    // index into label list of edge's label
   BOOLEAN directed; // TRUE if edge is directed
   BOOLEAN used;     // flag for marking edge used at various times
                     //   used flag assumed FALSE, so always reset when done
   BOOLEAN failed;
   ULONG map;
} ReferenceEdge;

// ReferenceVertex
typedef struct
{
   ULONG label;    // index into label list of vertex's label
   ULONG numEdges; // number of edges defined using this vertex
   ULONG *edges;   // indices into edge array of edges using this vertex
   ULONG map;      // used to store mapping of this vertex to corresponding
                   // vertex in another graph
   BOOLEAN used;   // flag for marking vertex used at various times
                   // used flag assumed FALSE, so always reset when done
   BOOLEAN vertexValid;
} ReferenceVertex;

// ReferenceGraph
typedef struct
{
   ULONG  numVertices; // number of vertices in graph
   ULONG  numEdges;    // number of edges in graph
   ReferenceVertex *vertices;   // array of graph vertices
   ReferenceEdge   *edges;      // array of graph edges
   ULONG vertexListSize; // memory allocated to vertices array
   ULONG edgeListSize;   // memory allocated to edges array
} ReferenceGraph;

// ReferenceInstanceList
typedef struct _ref_inst_list_node
{
   InstanceList *instanceList;
   ReferenceGraph *refGraph;
   BOOLEAN firstPass;
   BOOLEAN doExtend;
   struct _ref_inst_list_node *next;
} RefInstanceListNode;

typedef struct
{
   RefInstanceListNode *head;
} RefInstanceList;

typedef struct _avltablenode
{
   struct avl_table *vertexTree;
   Substructure *sub;
   struct _avltablenode *next;
} AvlTableNode;

typedef struct
{
   AvlTableNode *head;
} AvlTreeList;

typedef struct
{
   AvlTreeList *avlTreeList;
} InstanceVertexList;

// Singly-connected linked list of subarrays for each increment
typedef struct _increment
{
   SubList *subList;           // List of subs for an increment
   ULONG incrementNum;         // Increment in which these substructures were 
                               // discovered
   ULONG numPosVertices;       // Number of pos vertices in this increment
   ULONG numPosEdges;          // Number of pos edges in this increment
   ULONG numNegVertices;       // Number of neg vertices in this increment
   ULONG numNegEdges;          // Number of neg edges in this increment
   ULONG startPosVertexIndex;  // Index of the first vertex in this increment
   ULONG startPosEdgeIndex;    // Index of the first edge in this increment
   ULONG startNegVertexIndex;  // Index of the first vertex in this increment
   ULONG startNegEdgeIndex;    // Index of the first edge in this increment
   double numPosEgs;           // Number of pos examples in this increment
   double numNegEgs;           // DNumber of pos examples in this increment
} Increment;

typedef struct _increment_list_node
{
   Increment *increment;
   struct _increment_list_node *next;
} IncrementListNode;

typedef struct
{
   IncrementListNode *head;
} IncrementList;

// Parameters: parameters used throughout SUBDUE system
typedef struct 
{
   char inputFileName[FILE_NAME_LEN];   // main input file
   char psInputFileName[FILE_NAME_LEN]; // predefined substructures input file
   char outFileName[FILE_NAME_LEN];     // file for machine-readable output
   Graph *posGraph;      // Graph of positive examples
   Graph *negGraph;      // Graph of negative examples
   double posGraphDL;    // Description length of positive input graph
   double negGraphDL;    // Description length of negative input graph
   ULONG numPosEgs;      // Number of positive examples
   ULONG numNegEgs;      // Number of negative examples
   ULONG *posEgsVertexIndices; // vertex indices of where positive egs begin
   ULONG *negEgsVertexIndices; // vertex indices of where negative egs begin
   LabelList *labelList; // List of unique labels in input graph(s)
   Graph **preSubs;      // Array of predefined substructure graphs
   ULONG numPreSubs;     // Number of predefined substructures read in
   BOOLEAN predefinedSubs; // TRUE is predefined substructures given
   BOOLEAN outputToFile; // TRUE if file given for machine-readable output
   BOOLEAN directed;     // If TRUE, 'e' edges treated as directed
   ULONG beamWidth;      // Limit on size of substructure queue (> 0)
   ULONG limit;          // Limit on number of substructures expanded (> 0)
   ULONG maxVertices;    // Maximum vertices in discovered substructures
   ULONG minVertices;    // Minimum vertices in discovered substructures
   ULONG numBestSubs;    // Limit on number of best substructures
                         //   returned (> 0)
   BOOLEAN valueBased;   // If TRUE, then queues are trimmed to contain
                         //   all substructures with the top beamWidth
                         //   values; otherwise, queues are trimmed to
                         //   contain only the top beamWidth substructures.
   BOOLEAN prune;        // If TRUE, then expanded substructures with lower
                         //   value than their parents are discarded.
   ULONG outputLevel;    // More screen (stdout) output as value increases
   BOOLEAN allowInstanceOverlap; // Default is FALSE; if TRUE, then instances
                                 // may overlap, but compression costlier
   ULONG evalMethod;     // One of EVAL_MDL (default), EVAL_SIZE or
                         //   EVAL_SETCOVER
   double threshold;     // Percentage of size by which an instance can differ
                         // from the substructure definition according to
                         // graph match transformation costs
   ULONG iterations;     // Number of SUBDUE iterations; if more than 1, then
                         // graph compressed with best sub between iterations
   double *log2Factorial;   // Cache array A[i] = lg(i!); grows as needed
   ULONG log2FactorialSize; // Size of log2Factorial array
   ULONG numPartitions;  // Number of partitions used by parallel SUBDUE
   BOOLEAN recursion;    // If TRUE, recursive graph grammar subs allowed
   BOOLEAN variables;    // If TRUE, variable vertices allowed
   BOOLEAN relations;    // If TRUE, relations between vertices allowed
   BOOLEAN incremental;  // If TRUE, data is processed incrementally
   BOOLEAN compress;     // If TRUE, write compressed graph to file
   IncrementList *incrementList;   // Set of increments
   InstanceVertexList *vertexList; // List of avl trees containing
                                   // instance vertices
   ULONG posGraphSize;
   ULONG negGraphSize;
} Parameters;


//---------------------------------------------------------------------------
// Function Prototypes
//---------------------------------------------------------------------------

// compress.c

Graph *CompressGraph(Graph *, InstanceList *, Parameters *);
void AddOverlapEdges(Graph *, Graph *, InstanceList *, ULONG, ULONG, ULONG);
Edge *AddOverlapEdge(Edge *, ULONG *, ULONG, ULONG, ULONG);
Edge *AddDuplicateEdges(Edge *, ULONG *, Edge *, Graph *, ULONG, ULONG);
void CompressFinalGraphs(Substructure *, Parameters *, ULONG, BOOLEAN);
void CompressLabelListWithGraph(LabelList *, Graph *, Parameters *);
ULONG SizeOfCompressedGraph(Graph *, InstanceList *, Parameters *, ULONG);
ULONG NumOverlapEdges(Graph *, InstanceList *);
void RemovePosEgsCovered(Substructure *, Parameters *);
void MarkExample(ULONG, ULONG, Graph *, BOOLEAN);
void CopyUnmarkedGraph(Graph *, Graph *, ULONG, Parameters *);
void CompressWithPredefinedSubs(Parameters *);
void WriteCompressedGraphToFile(Substructure *sub, Parameters *parameters,
                                ULONG iteration);
void WriteUpdatedGraphToFile(Substructure *sub, Parameters *parameters);
void WriteUpdatedIncToFile(Substructure *sub, Parameters *parameters);
void MarkPosEgsCovered(Substructure *, Parameters *);

// discover.c

SubList *DiscoverSubs(Parameters *);
SubList *GetInitialSubs(Parameters *);
BOOLEAN SinglePreviousSub(Substructure *, Parameters *);

// dot.c

void WriteGraphToDotFile(char *, Parameters *);

void WriteGraphWithInstancesToDotFile(char *, Graph *, InstanceList *,
                                      Parameters *);
void WriteSubsToDotFile(char *, Graph **, ULONG, Parameters *);
void WriteVertexToDotFile(FILE *, ULONG, ULONG, Graph *, LabelList *, char *);
void WriteEdgeToDotFile(FILE *, ULONG, ULONG, Graph *, LabelList *, char *);

// evaluate.c

void EvaluateSub(Substructure *, Parameters *);
ULONG GraphSize(Graph *);
double MDL(Graph *, ULONG, Parameters *);
ULONG NumUniqueEdges(Graph *, ULONG);
ULONG MaxEdgesToSingleVertex(Graph *, ULONG);
double ExternalEdgeBits(Graph *, Graph *, ULONG);
double Log2Factorial(ULONG, Parameters *);
double Log2(ULONG);
ULONG PosExamplesCovered(Substructure *, Parameters *);
ULONG NegExamplesCovered(Substructure *, Parameters *);
ULONG ExamplesCovered(InstanceList *, Graph *, ULONG, ULONG *, ULONG);

// extend.c

SubList *ExtendSub(Substructure *, Parameters *);
InstanceList *ExtendInstances(InstanceList *, Graph *);
Instance *CreateExtendedInstance(Instance *, ULONG, ULONG, Graph *);
Substructure *CreateSubFromInstance(Instance *, Graph *);
void AddPosInstancesToSub(Substructure *, Instance *, InstanceList *, 
                          Parameters *, ULONG);
void AddNegInstancesToSub(Substructure *, Instance *, InstanceList *, 
                          Parameters *);
Substructure *RecursifySub(Substructure *, Parameters *);
Substructure *MakeRecursiveSub(Substructure *, ULONG, Parameters *);
InstanceList *GetRecursiveInstances(Graph *, InstanceList *, ULONG, ULONG);
void AddRecursiveInstancePair(ULONG, ULONG, Instance *, Instance *,
                              ULONG, Edge *, ULONG, Instance **);
InstanceList *CollectRecursiveInstances(Instance **, ULONG);

// graphmatch.c

BOOLEAN GraphMatch(Graph *, Graph *, LabelList *, double, double *,
                   VertexMap *);
double InexactGraphMatch(Graph *, Graph *, LabelList *, double, VertexMap *);
void OrderVerticesByDegree(Graph *, ULONG *);
ULONG MaximumNodes(ULONG);
double DeletedEdgesCost(Graph *, Graph *, ULONG, ULONG, ULONG *, LabelList *);
double InsertedEdgesCost(Graph *, ULONG, ULONG *);
double InsertedVerticesCost(Graph *, ULONG *);
MatchHeap *AllocateMatchHeap(ULONG);
VertexMap *AllocateNewMapping(ULONG, VertexMap *, ULONG, ULONG);
void InsertMatchHeapNode(MatchHeapNode *, MatchHeap *);
void ExtractMatchHeapNode(MatchHeap *, MatchHeapNode *);
void HeapifyMatchHeap(MatchHeap *);
BOOLEAN MatchHeapEmpty(MatchHeap *);
void MergeMatchHeaps(MatchHeap *, MatchHeap *);
void CompressMatchHeap(MatchHeap *, ULONG);
void PrintMatchHeapNode(MatchHeapNode *);
void PrintMatchHeap(MatchHeap *);
void ClearMatchHeap(MatchHeap *);
void FreeMatchHeap(MatchHeap *);

// graphops.c

void ReadInputFile(Parameters *);
ULONG *AddVertexIndex(ULONG *, ULONG, ULONG);
void ReadPredefinedSubsFile(Parameters *);
Graph **ReadSubGraphsFromFile(char *, char *, ULONG *, Parameters *);
Graph *ReadGraph(char *, LabelList *, BOOLEAN);
void ReadVertex(Graph *, FILE *, LabelList *, ULONG *, ULONG);
void AddVertex(Graph *, ULONG);
void ReadEdge(Graph *, FILE *, LabelList *, ULONG *, BOOLEAN, ULONG);
void AddEdge(Graph *, ULONG, ULONG, BOOLEAN, ULONG, BOOLEAN);
void StoreEdge(Edge *, ULONG, ULONG, ULONG, ULONG, BOOLEAN, BOOLEAN);
void AddEdgeToVertices(Graph *, ULONG);
int ReadToken(char *, FILE *, ULONG *);
ULONG ReadLabel(FILE *, LabelList *, ULONG *);
ULONG ReadInteger(FILE *, ULONG *);
Graph *AllocateGraph(ULONG, ULONG);
Graph *CopyGraph(Graph *);
void FreeGraph(Graph *);
void PrintGraph(Graph *, LabelList *);
void PrintVertex(Graph *, ULONG, ULONG, LabelList *);
void PrintEdge(Graph *, ULONG, ULONG, LabelList *);
void WriteGraphToFile(FILE *, Graph *, LabelList *, ULONG, ULONG, ULONG, BOOLEAN);

// labels.c

LabelList *AllocateLabelList(void);
ULONG StoreLabel(Label *, LabelList *);
ULONG GetLabelIndex(Label *, LabelList *);
ULONG SubLabelNumber(ULONG, LabelList *);
double LabelMatchFactor(ULONG, ULONG, LabelList *);
void PrintLabel(ULONG, LabelList *);
void PrintLabelList(LabelList *);
void FreeLabelList(LabelList *);
void WriteLabelToFile(FILE *, ULONG, LabelList *, BOOLEAN);

// mpi.c
Substructure *ReceiveBestSubFromChild(int *, Parameters *);
BOOLEAN UniqueChildSub(Substructure *, Substructure **, Parameters *);
void SendEvalSubToChild(Substructure *, Parameters *, int);
void ReceiveEvalFromChild(double *, ULONG *, ULONG *);
void SendBestSubToChildren(Substructure *, Parameters *);
void SendBestSubToMaster(Substructure *, Parameters *);
Substructure *ReceiveEvalSubFromMaster(Parameters *);
void SendEvalToMaster(double, ULONG, ULONG);
Substructure *ReceiveBestSubFromMaster(Parameters *);
Substructure *FindEvalSub(Substructure *, SubList *, Parameters *);
void PackSubstructure(Substructure *, Parameters *, char *, int *);
void PackGraph(Graph *, Parameters *, char *, int *);
void PackLabel(ULONG, LabelList *, char *, int *);
Substructure *UnpackSubstructure(char *, int *, Parameters *);
Graph *UnpackGraph(char *, int *, Parameters *);
ULONG UnpackLabel(char *, int *, LabelList *);

// sgiso.c

InstanceList *FindInstances(Graph *, Graph *, Parameters *);
InstanceList *FindSingleVertexInstances(Graph *, Vertex *, Parameters *);
InstanceList *ExtendInstancesByEdge(InstanceList *, Graph *, Edge *,
                                    Graph *, Parameters *);
BOOLEAN EdgesMatch(Graph *, Edge *, Graph *, Edge *, Parameters *);
InstanceList *FilterInstances(Graph *, InstanceList *, Graph *,
                              Parameters *);

// subops.c

Substructure *AllocateSub(void);
void FreeSub(Substructure *);
void PrintSub(Substructure *, Parameters *);
SubListNode *AllocateSubListNode(Substructure *);
void FreeSubListNode(SubListNode *);
SubList *AllocateSubList(void);
void SubListInsert(Substructure *, SubList *, ULONG, BOOLEAN, LabelList *);
BOOLEAN MemberOfSubList(Substructure *, SubList *, LabelList *);
void FreeSubList(SubList *);
void PrintSubList(SubList *, Parameters *);
void PrintNewBestSub(Substructure *, SubList *, Parameters *);
ULONG CountSubs(SubList *);
Instance *AllocateInstance(ULONG, ULONG);
void FreeInstance(Instance *);
void PrintInstance(Instance *, ULONG, Graph *, LabelList *);
void MarkInstanceVertices(Instance *, Graph *, BOOLEAN);
void MarkInstanceEdges(Instance *, Graph *, BOOLEAN);
InstanceListNode *AllocateInstanceListNode(Instance *);
void FreeInstanceListNode(InstanceListNode *);
InstanceList *AllocateInstanceList(void);
void FreeInstanceList(InstanceList *);
void PrintInstanceList(InstanceList *, Graph *, LabelList *);
void PrintPosInstanceList(Substructure *, Parameters *);
void PrintNegInstanceList(Substructure *, Parameters *);
ULONG InstanceExampleNumber(Instance *, ULONG *, ULONG);
ULONG CountInstances(InstanceList *);
void InstanceListInsert(Instance *, InstanceList *, BOOLEAN);
BOOLEAN MemberOfInstanceList(Instance *, InstanceList *);
BOOLEAN InstanceMatch(Instance *, Instance *);
BOOLEAN InstanceOverlap(Instance *, Instance *);
BOOLEAN InstanceListOverlap(Instance *, InstanceList *);
BOOLEAN InstancesOverlap(InstanceList *);
Graph *InstanceToGraph(Instance *, Graph *);
BOOLEAN InstanceContainsVertex(Instance *, ULONG);
void AddInstanceToInstance(Instance *, Instance *);
void AddEdgeToInstance(ULONG, Edge *, Instance *);
BOOLEAN NewEdgeMatch(Graph *, Instance *, Graph *, Instance *, 
                     Parameters *, double, double *);
void UpdateMapping(Instance *, Instance *);

// test.c

void Test(char *, char *, Parameters *, ULONG *, ULONG *, ULONG *, ULONG *);
BOOLEAN PositiveExample(Graph *, Graph **, ULONG, Parameters *);

// utility.c

void OutOfMemoryError(char *);
void PrintBoolean(BOOLEAN);

// inccomp.c

double ComputeValue(IncrementListNode *, Substructure *, LabelList *, ULONG *,
                    ULONG *, ULONG *, ULONG *, Parameters *);
void InsertSub(SubList *, Substructure *, double, ULONG, ULONG);
void AdjustMetrics(Substructure *sub, Parameters *parameters);
SubList *ComputeBestSubstructures(Parameters *parameters, int listSize);

// incutil.c

void AddInstanceToTree(struct avl_table *, Instance *);
void AddInstanceVertexList(Substructure *, struct avl_table *, Parameters *);
void AddNewIncrement(ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG,
                     Parameters *);
int compare_ints(const void *, const void *);
Increment *GetCurrentIncrement(Parameters *);
ULONG GetCurrentIncrementNum(Parameters *);
ULONG GetStartVertexIndex(ULONG, Parameters *, ULONG);
Increment *GetIncrement(ULONG, Parameters *);
ULONG IncrementSize(Parameters *, ULONG, ULONG);
ULONG IncrementNumExamples(Parameters *, ULONG, ULONG);
struct avl_table *GetSubTree(Substructure *, Parameters *);
void PrintStoredSubList(SubList *, Parameters *);
void PrintStoredSub(Substructure *, Parameters *);
void SetIncrementNumExamples(Parameters *);
void StoreSubs(SubList *, Parameters *);
Substructure * CopySub(Substructure *);
void WriteResultsToFile(SubList *, FILE *, Increment *, Parameters *);
char *GetOutputFileName(char *, ULONG);
IncrementListNode *GetIncrementListNode(ULONG, Parameters *);
void AddVertexTrees(SubList *, Parameters *);

// gendata.c

BOOLEAN GetNextIncrement(Parameters *parameters);
void InitializeGraph(Parameters *parameters);
BOOLEAN CreateFromFile(Parameters *parameters, ULONG startPosVertexIndex,
                       ULONG startNegVertexIndex);
BOOLEAN ReadIncrement(char *filename, Parameters *parameters,
                      ULONG startPosVertex, ULONG startNegVertex);
void ReadIncrementVertex(Graph *graph, FILE *fp, LabelList *labelList,
                     ULONG *pLineNo, ULONG vertexOffset);
void ReadIncrementEdge(Graph *graph, FILE *fp, LabelList *labelList,
                       ULONG *pLineNo, BOOLEAN directed,
                       ULONG startVertexIndex, ULONG vertexOffset);

// incboundary.c

InstanceList *EvaluateBoundaryInstances(SubList *bestSubList,
                                        Parameters *parameters);
void ProcessInstancesForSub(Substructure *bestSub,
                            RefInstanceList *refInstanceList,
                            InstanceList *finalInstanceList,
                            Parameters *parameters);
InstanceList *PruneCandidateList(Substructure *bestSub,
                                 ReferenceGraph *refGraph,
                                 InstanceList *instanceList,
                                 Parameters *parameters);
void CreateExtendedGraphs(RefInstanceListNode *refInstanceListNode,
                          Substructure *bestSub, Graph *graph,
                          Parameters *parameters);
BOOLEAN FindInitialBoundaryInstances(SubList *bestSubList, Graph *graph,
                                     LabelList *labelList,
                                     ULONG startVertexIndex,
                                     Parameters *parameters);
BOOLEAN CheckForSeedInstanceOverlap(Instance *candidateInstance,
                                    InstanceList *instanceList);
BOOLEAN ExtendBoundaryInstances(Substructure *bestSub,
                                RefInstanceListNode *refInstanceListNode,
                                Parameters *parameters);
void ProcessExtendedInstances(InstanceList *extendedInstances,
                              InstanceList *candidateInstances,
                              Substructure *bestSub, ReferenceGraph *refGraph,
                              Graph *fullGraph, Parameters *parameters);
void RemoveFromList(RefInstanceList *refInstanceList,
                    RefInstanceListNode **currentNode);
void MarkVertices(ReferenceGraph *refGraph, ULONG v1, ULONG v2);
BOOLEAN AddInstanceToIncrement(Substructure *newSub, Instance *instance,
                               Parameters *parameters);
BOOLEAN CheckInstanceForOverlap(Instance *instance, Substructure *newSub,
                                Parameters *parameters);
BOOLEAN CheckVertexForOverlap(ULONG vertex, Substructure *sub,
                              Parameters *parameters);
BOOLEAN CheckForSubgraph(Graph *g1, Graph *g2, Parameters *parameters);
BOOLEAN CheckForMatch(Graph *subGraph, InstanceList *instanceList, Graph *graph,
                      Parameters *parameters);

// incgraphops.c

void MarkGraphEdgesUsed(ReferenceGraph *refGraph, Graph *fullGraph,
                        BOOLEAN value);
void MarkGraphEdgesValid(ReferenceGraph *refGraph, Graph *fullGraph,
                         BOOLEAN mark);
void MarkRefGraphInstanceEdges(Instance *instance, ReferenceGraph *refGraph,
                               BOOLEAN value);
ReferenceGraph *CopyReferenceGraph(ReferenceGraph *g);
void AddReferenceVertex(ReferenceGraph *graph, ULONG labelIndex);
void AddReferenceEdge(ReferenceGraph *graph, ULONG sourceVertexIndex,
                      ULONG targetVertexIndex, BOOLEAN directed,
                      ULONG labelIndex, BOOLEAN spansIncrement);
void FreeReferenceGraph (ReferenceGraph *graph);
ReferenceGraph *AllocateReferenceGraph (ULONG v, ULONG e);
void AddRefEdgeToRefVertices(ReferenceGraph *graph, ULONG edgeIndex);
ReferenceGraph *InstanceToRefGraph(Instance *instance, Graph *graph, 
                                   Parameters *parameters);
Instance *CreateGraphRefInstance(Instance *instance1, ReferenceGraph *refGraph);
void SortIndices(ULONG *A, long p, long r);
long Partition(ULONG *A, long p, long r);
RefInstanceListNode *AllocateRefInstanceListNode();
RefInstanceList *AllocateRefInstanceList();
void FreeRefInstanceListNode(RefInstanceListNode *refInstanceListNode);
Instance * CopyInstance(Instance *instance);
BOOLEAN VertexInSubList(SubList *subList, Vertex *vertex);
BOOLEAN VertexInSub(Graph *subDef, Vertex *vertex);

// incextend.c

ReferenceGraph *ExtendRefGraph(ReferenceGraph *refGraph, Substructure *bestSub,
                               Graph *fullGraph, Parameters *parameters);
InstanceList *ExtendConstrainedInstance(Instance *instance,
                                        Substructure *bestSub, 
                                        ReferenceGraph *refGraph, 
                                        Graph *fullGraph,
                                        Parameters *parameters);
Instance *CreateConstrainedExtendedInstance(Instance *instance, ULONG v,
                                            ULONG v2, ULONG e, 
                                            ReferenceGraph *graph);

#endif
