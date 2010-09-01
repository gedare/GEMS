#define NUM_COMBINATIONS 7
#define NUM_VERSIONS 15
#define NUM_TESTS 4

int minterms[NUM_COMBINATIONS];
int minterm_versions[NUM_COMBINATIONS][NUM_VERSIONS];
int correct_output[NUM_COMBINATIONS];
int version;

int retries[NUM_TESTS];
int profiled[NUM_TESTS];

void abort_once(int abortion_group);
void initialize();
void print_usage(char* exe);
void take_snapshot(int profile_group);
