#include "fsi.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "macros.h"

/*
 * FSI_INIT
 * 	intializes the ParticleState struct from the parameters defined in
 * 	params
 */
void fsi_init_state(FsiParams * params, ParticleState * p_state) {
	unsigned int dim_it, k;

	// initial condition
	p_state->angle = params->init_angle;
	p_state->ang_vel = params->init_ang_vel;

	// Compute inertia
	p_state->inertia = params->rho * PI * params->a * params->b *
						(pow2(params->a) + pow2(params->b)) / 4.0;

	// Copy center point
	p_state->coord_c[0] = params->coord_c[0];
	p_state->coord_c[1] = params->coord_c[1];

	p_state->width = params->a;

	// Allocate space for particle arrays
	p_state->nodes = params->nodes;

	// Vector quantities
	for(dim_it = 0; dim_it < DIM; ++dim_it) {
		// FSI force
		p_state->force_fsi[dim_it] = (double *) malloc(p_state->nodes * sizeof(double));

		// Node positions
		p_state->coord_p[dim_it] = (double *) malloc(p_state->nodes * sizeof(double));
		p_state->coord_a[dim_it] = (double *) malloc(p_state->nodes * sizeof(double));
	}

	// Scalar quantities
	p_state->volume = (double *) malloc(p_state->nodes * sizeof(double));

	generate_particle_initial(params, p_state);
	generate_particle_volume(p_state);
	fsi_update_particle_nodes(p_state);
	//print_particle(p_state);
}

void generate_particle_initial(FsiParams * params, ParticleState * p_state)
{
	double apb, amb, adiv, sqt, circ, optimal_spacing, xt, yt, xtest, ytest,
		beta, dbeta, dFPN, a, b;
	unsigned int nc;

	a = params->a;
	b = params->b;

	// Find the optimal spacing
	apb = a + b;
	amb = a - b;
	adiv = amb/apb;
	sqt = 3.0*adiv*adiv;
	circ = PI * apb * (1.0 + sqt/(10.0+sqrt(4.0-sqt)));
	optimal_spacing = circ / p_state->nodes;

	// Generate node positions
	p_state->coord_a[0][0] = xt = a;
	p_state->coord_a[1][0] = yt = 0.0;
	dFPN = 0.0;
	dbeta = 0.0001;
	beta = 0.0;
	nc = 1;

	while(nc < p_state->nodes) {
		beta += dbeta;
		xtest = a*cos(beta);
		ytest = b*sin(beta);
		dFPN = dFPN+sqrt(pow2(xtest-xt)+pow2(ytest-yt));

		if(dFPN > optimal_spacing) {
			p_state->coord_a[0][nc] = xtest;
			p_state->coord_a[1][nc] = ytest;
			dFPN = 0.0;
			nc = nc+1;
		}

		xt = xtest;
		yt = ytest;
	}
}

inline double pow2(double X)
{
	return X*X;
}

void fsi_update_particle_nodes(ParticleState * p_state)
{
	unsigned int i;
	for(i = 0; i < p_state->nodes; ++i) {
		p_state->coord_p[0][i] =
				p_state->coord_a[0][i] * cos(p_state->angle) -
				p_state->coord_a[1][i] * sin(p_state->angle) +
				p_state->coord_c[0];
		p_state->coord_p[1][i] =
				p_state->coord_a[0][i] * sin(p_state->angle) +
				p_state->coord_a[1][i] * cos(p_state->angle) +
				p_state->coord_c[1];
	}
}

void generate_particle_volume(ParticleState * p_state) {
	double ram, rap, hpn, area;
	unsigned int i, i_prev, i_next;

	for(i = 0; i < p_state->nodes; ++i) {
		i_prev = (i == 0) 					? p_state->nodes-1 	: i-1;
		i_next = (i == (p_state->nodes-1)) 	? 0 				: i+1;

		ram = sqrt(pow2(p_state->coord_a[0][i] - p_state->coord_a[0][i_prev])
				+ pow2(p_state->coord_a[1][i] - p_state->coord_a[1][i_prev]));
		rap = sqrt(pow2(p_state->coord_a[0][i] - p_state->coord_a[0][i_next])
				+ pow2(p_state->coord_a[1][i] - p_state->coord_a[1][i_next]));
		hpn = 0.5 * (ram + rap);
		area = hpn * hpn;
		p_state->volume[i] = area * sqrt(area);
	}
}

void print_particle(ParticleState * p_state)
{
	FILE * file = fopen("particle_initial.txt", "w");
	unsigned int i;
	for(i = 0; i < p_state->nodes; ++i)
		fprintf(file, "%.18f %.18f\n", p_state->coord_p[0][i], p_state->coord_p[1][i]);
	fclose(file);
}

void fsi_print_info(ParticleState * p_state)
{
	printf("Fsi parameters:\n");
	printf("Nodes: %d\n", p_state->nodes);
	printf("Inertia: %f\n", p_state->inertia);
	printf("Particle length: %f\n", p_state->width);
	printf("Particle volume (first node): %f\n", p_state->volume[0]);
}

void fsi_destroy_state(ParticleState * p_state)
{
	unsigned int dim_it;
	for(dim_it = 0; dim_it < DIM; ++dim_it) {
		free(p_state->force_fsi[dim_it]);
		free(p_state->coord_p[dim_it]);
		free(p_state->coord_a[dim_it]);
	}

	// Scalar quantities
	free(p_state->volume);
}

void fsi_copy_state(ParticleState * dest, ParticleState * src)
{
	unsigned int dim_it, j;

	// Copy scalar quantities
	dest->ang_vel = src->ang_vel;
	dest->angle = src->angle;
	dest->inertia = src->inertia;
	dest->nodes = src->nodes;
	dest->torque = src->torque;
	dest->width = src->width;

	// Copy center point coordinate
	dest->coord_c[0] = src->coord_c[0];
	dest->coord_c[1] = src->coord_c[1];

	// Allocate space for vector quantites
	for(dim_it = 0; dim_it < DIM; ++dim_it) {
		// FSI force
		dest->force_fsi[dim_it] = (double *) malloc(dest->nodes * sizeof(double));

		// Node positions
		dest->coord_p[dim_it] = (double *) malloc(dest->nodes * sizeof(double));
		dest->coord_a[dim_it] = (double *) malloc(dest->nodes * sizeof(double));
	}

	// Scalar quantities
	dest->volume = (double *) malloc(dest->nodes * sizeof(double));

	// Copy values
	for(j = 0; j < dest->nodes; ++j) {
		dest->volume[j] = src->volume[j];

		for(dim_it = 0; dim_it < DIM; ++dim_it ) {
			dest->force_fsi[dim_it][j] = src->force_fsi[dim_it][j];
			dest->coord_p[dim_it][j] = src->coord_p[dim_it][j];
			dest->coord_a[dim_it][j] = src->coord_a[dim_it][j];
		}
	}
}

void fsi_run(FlowState * f_state, ParticleState * p_state)
{
	fsi_compute_force_on_particle(f_state, p_state);
	fsi_update_particle(p_state);
	fsi_project_force_on_fluid(f_state, p_state);
}

/**
 * fsi_compute_force_on_particle
 * Evaluates the torque and forces acting on the particle from the surrounding fluid.
 */
void fsi_compute_force_on_particle(FlowState * f_state, ParticleState * p_state)
{
	unsigned int np, i, j, idx, i_min, i_max, j_min, j_max;
	double dir, dx, dy, up_particle_x, up_particle_y, uf_particle_x, uf_particle_y;

	p_state->torque = 0.0;

	for(np = 0; np < p_state->nodes; ++np) {
		// Find positions relative to the center
		dx = p_state->coord_p[0][np] - p_state->coord_c[0];
		dy = p_state->coord_p[1][np] - p_state->coord_c[1];

		// Particle velocity at node np
		up_particle_x = -dy * p_state->ang_vel;
		up_particle_y = dx * p_state->ang_vel;

		// Compute the fluid velocity at node np
		uf_particle_x = uf_particle_y = 0;

		// Determine indices in which the contributing fluid nodes are contained
		i_min = floor(p_state->coord_p[0][np] - 2);
		i_max = ceil(p_state->coord_p[0][np] + 2);
		j_min = floor(p_state->coord_p[1][np] - 2);
		j_max = ceil(p_state->coord_p[1][np] + 2);

		for(i = i_min; i <= i_max; ++i) {
			for(j = j_min; j <= j_max; ++j) {
				idx = i*f_state->ly + j;
				dir =  dirac(i - p_state->coord_p[0][np], j - p_state->coord_p[1][np]);

				uf_particle_x += f_state->u[0][idx] * dir;
				uf_particle_y += f_state->u[1][idx] * dir;
			}
		}

		//Evaluate the force on the ellipsoid
		p_state->force_fsi[0][np] = (uf_particle_x - up_particle_x) * p_state->volume[np];
		p_state->force_fsi[1][np] = (uf_particle_y - up_particle_y) * p_state->volume[np];

		// Compute the torque addition
		p_state->torque += dx * p_state->force_fsi[1][np] - dy * p_state->force_fsi[0][np];
	}
}

double dirac(double dx, double dy)
{
	if((dx*dx+dy*dy) > 4)
		return 0;
	else
		return (1.0/16.0) * (1.0 + cos(PI*dx/2.0)) * (1.0 + cos(PI*dy/2.0));
}

void fsi_update_particle(ParticleState * p_state)
{
	// Update the particle angle and angular velocity
	p_state->ang_vel += p_state->torque / p_state->inertia;
	p_state->angle += p_state->ang_vel;

	// Update the particle nodes' positions
	fsi_update_particle_nodes(p_state);
}

void fsi_project_force_on_fluid(FlowState * f_state, ParticleState * p_state)
{
	unsigned int i, j, np, idx, i_min, i_max, j_min, j_max;
	double temp_x, temp_y, dir;

	//Calculate force on fluid
	i_min = floor(p_state->coord_c[0] - p_state->width - 2);
	i_max = ceil(p_state->coord_c[0] + p_state->width + 2);
	j_min = floor(p_state->coord_c[1] - p_state->width - 2);
	j_max = ceil(p_state->coord_c[1] + p_state->width + 2);

	for(i = i_min; i <= i_max; ++i) {
		for(j = j_min; j <= j_max; ++j) {
			idx = i*f_state->ly + j;

			temp_x = 0;
			temp_y = 0;

			for(np=0; np < p_state->nodes; ++np) {
				dir =  dirac(i - p_state->coord_p[0][np], j - p_state->coord_p[1][np]);
				temp_x -= p_state->force_fsi[0][np] * dir;
				temp_y -= p_state->force_fsi[1][np] * dir;
			}

			f_state->force[0][idx] = temp_x;
			f_state->force[1][idx] = temp_y;
		}
	}
}
