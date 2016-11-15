#include <stdio.h>
#include <sys/resource.h>

int main() {
	struct rlimit lim;

	if (0 != getrlimit(RLIMIT_STACK, &lim)) {
		fprintf(stderr, "failed to get RLIMIT_STACK resource limit\n");
		return 1;
	}
	printf("stack size: %ld\n", lim.rlim_cur);

	if (0 != getrlimit(RLIMIT_NPROC, &lim)) {
		fprintf(stderr, "failed to get RLIMIT_NPROC resource limit\n");
		return 1;
	}
	printf("process limit: %ld\n", lim.rlim_cur);

	if (0 != getrlimit(RLIMIT_NOFILE, &lim)) {
		fprintf(stderr, "failed to get RLIMIT_NOFILE resource limit\n");
		return 1;
	}
	printf("max file descriptors: %ld\n", lim.rlim_cur);

	return 0;
}
