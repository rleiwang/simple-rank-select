#define DEBUG_EVAL_COUNTS
#define LIBXML_XPTR_ENABLED
#undef XPATH_STREAMING

typedef enum {
    XPATH_OP_END=0,
    XPATH_OP_AND,
    XPATH_OP_OR,
    XPATH_OP_EQUAL,
    XPATH_OP_CMP,
    XPATH_OP_PLUS,
    XPATH_OP_MULT,
    XPATH_OP_UNION,
    XPATH_OP_ROOT,
    XPATH_OP_NODE,
    XPATH_OP_RESET, /* 10 */
    XPATH_OP_COLLECT,
    XPATH_OP_VALUE, /* 12 */
    XPATH_OP_VARIABLE,
    XPATH_OP_FUNCTION,
    XPATH_OP_ARG,
    XPATH_OP_PREDICATE,
    XPATH_OP_FILTER, /* 17 */
    XPATH_OP_SORT /* 18 */
#ifdef LIBXML_XPTR_ENABLED
    ,XPATH_OP_RANGETO
#endif
} xmlXPathOp;

typedef enum {
    AXIS_ANCESTOR = 1,
    AXIS_ANCESTOR_OR_SELF,
    AXIS_ATTRIBUTE,
    AXIS_CHILD,
    AXIS_DESCENDANT,
    AXIS_DESCENDANT_OR_SELF,
    AXIS_FOLLOWING,
    AXIS_FOLLOWING_SIBLING,
    AXIS_NAMESPACE,
    AXIS_PARENT,
    AXIS_PRECEDING,
    AXIS_PRECEDING_SIBLING,
    AXIS_SELF
} xmlXPathAxisVal;

typedef enum {
    NODE_TEST_NONE = 0,
    NODE_TEST_TYPE = 1,
    NODE_TEST_PI = 2,
    NODE_TEST_ALL = 3,
    NODE_TEST_NS = 4,
    NODE_TEST_NAME = 5
} xmlXPathTestVal;

typedef enum {
    NODE_TYPE_NODE = 0,
    NODE_TYPE_COMMENT = XML_COMMENT_NODE,
    NODE_TYPE_TEXT = XML_TEXT_NODE,
    NODE_TYPE_PI = XML_PI_NODE
} xmlXPathTypeVal;

typedef struct _xmlXPathStepOp xmlXPathStepOp;
typedef xmlXPathStepOp *xmlXPathStepOpPtr;
struct _xmlXPathStepOp {
    xmlXPathOp op;              /* The identifier of the operation */
    int ch1;                    /* First child */
    int ch2;                    /* Second child */
    int value;
    int value2;
    int value3;
    void *value4;
    void *value5;
    void *cache;
    void *cacheURI;
};

struct _xmlXPathCompExpr {
    int nbStep;                 /* Number of steps in this expression */
    int maxStep;                /* Maximum number of steps allocated */
    xmlXPathStepOp *steps;      /* ops for computation of this expression */
    int last;                   /* index of last step in expression */
    xmlChar *expr;              /* the expression being computed */
    xmlDictPtr dict;            /* the dictionnary to use if any */
#ifdef DEBUG_EVAL_COUNTS
    int nb;
    xmlChar *string;
#endif
#ifdef XPATH_STREAMING
    xmlPatternPtr stream;
#endif
};
