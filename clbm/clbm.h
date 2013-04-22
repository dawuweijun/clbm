#ifndef CLBM_H_
#define CLBM_H_
#include "data_types.h"

typedef struct {
	double f, tau, u_max, rho;
} FlowParams;

void lbm_init(FlowState *, LbmState *);
void lbm_run(FlowState *, LbmState *);
void lbm_destroy();
void lbm_lattice_info();
void lbm_print_info();

static void bcs(FlowState * , LbmState *);
static void collide(FlowState * , LbmState *);
static void stream(FlowState * , LbmState *);
static void hydrovar(FlowState * , LbmState *);
static void eval_hydrovar(FlowState *, LbmState *, Node *);
static void implement_bcs(FlowState * , LbmState *);

#endif /* CLBM_H_ */
