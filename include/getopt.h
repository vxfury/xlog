#ifndef GETOPT_H
#define GETOPT_H

/* include files needed by this include file */

#define no_argument			0
#define required_argument	1
#define optional_argument	2

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* types defined by this include file */
struct option {
	const char *name;	/* the name of the long option */
	int has_arg;		/* one of the above macros */
	int *flag;			/* determines if getopt_long() returns a
						 * value for a long option; if it is
						 * non-NULL, 0 is returned as a function
						 * value and the value of val is stored in
						 * the area pointed to by flag.  Otherwise,
						 * val is returned. */
	int val;			/* determines the value to return if flag is
						 * NULL. */

};

/* While getopt.h is a glibc extension, the following are newlib extensions.
 * They are optionally included via the __need_getopt_newlib flag.  */

/* macros defined by this include file */
#define NO_ARG          	no_argument
#define REQUIRED_ARG    	required_argument
#define OPTIONAL_ARG    	optional_argument

/* The getopt_data structure is for reentrancy. Its members are similar to
   the externally-defined variables.  */
typedef struct getopt_data {
	char *optarg;
	int optind, opterr, optopt, optwhere;
} getopt_data;

/* function prototypes */
int getopt_r(
	int __argc, char *const __argv[], const char *__optstring,
	struct getopt_data *__data
);

int getopt_long_r(
	int __argc, char *const __argv[], const char *__shortopts,
	const struct option *__longopts, int *__longind,
	struct getopt_data *__data
);

int getopt_long_only_r(
	int __argc, char *const __argv[], const char *__shortopts,
	const struct option *__longopts, int *__longind,
	struct getopt_data *__data
);

#ifdef __cplusplus
};
#endif /* __cplusplus  */

#endif /* GETOPT_H */

/* END OF FILE getopt.h */
