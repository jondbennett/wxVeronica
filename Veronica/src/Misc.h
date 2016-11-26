// Truly misc. It is easier to hide them here than
// in the other header files that define classes and
// such in order to avoid having to include the other
// headers that such includes require.

// Compare two values to see if they are 'effectively equal'
#define EFFECTIVELY_EQUAL(v1, v2) ((fabs(v1-v2) < 0.02))
