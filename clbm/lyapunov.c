#include "lyapunov.h"
#include "fsi.h"
#include <math.h>
#include <stdlib.h>

void lyapunov_init_state(unsigned int it, LyapunovState * lp_state, ParticleState * p_state, FlowState * f_state, LbmState * lbm_state)
{
	// Initial disturbance and tolerance
	lp_state->d0 = 1.0e-6;
	lp_state->norm_tol = 1.0e-1;

	// Initialize scalar values
	lp_state->cum_sum = 0;
	lp_state->t0 = it;
	lp_state->lambda = 0;
	lp_state->norm_count = 0;

	// Save a pointer to the base state
	lp_state->base_particle_state = p_state;

	// Copy the state and introduce a small disturbance
	lp_state->perturbed_flow_state = (FlowState *) malloc(sizeof(FlowState));
	flow_clone_state(lp_state->perturbed_flow_state, f_state);
	lp_state->perturbed_lbm_state = (LbmState *) malloc(sizeof(FlowState));
	lbm_clone_state(lp_state->perturbed_lbm_state, lbm_state);
	lp_state->perturbed_particle_state = (ParticleState *) malloc(sizeof(ParticleState));
	fsi_clone_state(lp_state->perturbed_particle_state, p_state);
	lp_state->perturbed_particle_state->angle += lp_state->d0;
	fsi_update_particle_nodes(lp_state->perturbed_particle_state);
}

void lyapunov_destroy_state(LyapunovState * lp_state)
{
	fsi_free_state(lp_state->perturbed_particle_state);
	free(lp_state->perturbed_particle_state);
	lp_state->base_particle_state = NULL;
	flow_free_state(lp_state->perturbed_flow_state);
	free(lp_state->perturbed_flow_state);
	lbm_free_state(lp_state->perturbed_lbm_state);
}

void lyapunov_run(unsigned int it, LyapunovState * lp_state)
{
	double d, d_square, alpha_square, alpha;

	fsi_run(lp_state->perturbed_flow_state);
	lbm_run(lp_state->perturbed_flow_state);

	// Find the distance in phase space between the base and the perturbed state
	d = sqrt(pow(lp_state->perturbed_particle_state->angle - lp_state->base_particle_state->angle, 2) +
			   pow((lp_state->perturbed_particle_state->ang_vel/f_state->G - lp_state->base_particle_state->ang_vel/f_state->G), 2));
	alpha = d / lp_state->d0;

	// Normalize if the distance between the trajectories is too great
	if((d < lp_state->d0*lp_state->norm_tol) || d > (lp_state->d0/lp_state->norm_tol)) {
		// Push the perturbed orbit towards the base orbit
		lp_state->perturbed_particle_state->angle = lp_state->base_particle_state->angle + (lp_state->perturbed_particle_state->angle - lp_state->base_particle_state->angle) / alpha;
		lp_state->perturbed_particle_state->ang_vel = lp_state->base_particle_state->ang_vel + (lp_state->perturbed_particle_state->ang_vel*f_state->G - lp_state->base_particle_state->ang_vel*f_state->G) / alpha;

		fsi_update_particle_nodes(lp_state->perturbed_particle_state);

		// Update the lyapunov exponent
		lp_state->cum_sum += log(alpha);
		lp_state->lambda = lp_state->cum_sum / (f_state->G*((double)it - lp_state->t0));
		lp_state->norm_count++;
	}
}
