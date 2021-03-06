#include "output.h"
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>

extern int errno;

int do_mkdir(char *, mode_t);

void output_init(OutputParams * output_params)
{
	/* Create output folder structure */
	char foldercopy[256];
	strcpy(foldercopy, output_params->output_folder);
	char * start, * end;

	int status = 0;
	start = foldercopy;

	while(status == 0 && (end = strchr(start, '/')) != 0) {
		if(start != end) {
			*end = '\0';
			status = do_mkdir(foldercopy, 0777);
			*end = '/';
		}

		start = end + 1;
	}

	if(status == 0)
		do_mkdir(output_params->output_folder, 0777);

	/* Create file handle for particle data */
	output_params->output_file = NULL;
	char filename[256];

	if(output_params->print_particle_state) {
		sprintf(filename, "%s/particle.txt", output_params->output_folder);
		output_params->output_file = fopen(filename, "a+");
	}
}

int do_mkdir(char * path, mode_t mode)
{
	struct stat st;
	int status = 0;

	if (stat(path, &st) != 0) {
		if (mkdir(path, mode) != 0 && errno != EEXIST)
			status = -1;
	} else if (!S_ISDIR(st.st_mode)) {
		errno = ENOTDIR;
		status = -1;
	}

	return status;
}

void output_destroy(OutputParams * output_params)
{
	if(output_params->output_file) {
		fclose(output_params->output_file);
		output_params->output_file = NULL;
	}
}

void output_write_parameters_to_file(OutputParams * output_params, InputParameters * input_params)
{
	char file_name[512];
	sprintf(file_name, "%s/parameters.txt", output_params->output_folder);

	FILE * handle = fopen(file_name, "w");

	fprintf(handle, "lx\t%d\n", input_params->lx);
	fprintf(handle, "ly\t%d\n", input_params->ly);
	fprintf(handle, "timesteps\t%d\n", input_params->timesteps);
	fprintf(handle, "Re_p\t%f\n", input_params->Re_p);
	fprintf(handle, "conf\t%f\n", input_params->conf);
	fprintf(handle, "St\t%f\n", input_params->alpha * input_params->Re_p);
	fprintf(handle, "kb\t%f\n", input_params->kb);
	fprintf(handle, "freq\t%f\n", input_params->freq);
	fprintf(handle, "umax\t%f\n", input_params->u_max);
	fprintf(handle, "angle\t%f\n", input_params->init_angle);
	fprintf(handle, "ang_vel\t%f\n", input_params->init_ang_vel);

	fclose(handle);
}

void output_write_state_to_file(unsigned int it, OutputParams * output_params, FlowState * f_state, ParticleState * p_state, LyapunovState * lp_state)
{
	char file_name[256];
	if(output_params->print_particle_state) {
		if(lp_state)
			fprintf(output_params->output_file, "%.14g\t%.14g\t%.14g\t%.14g\n", (double)it * f_state->G, p_state->angle, p_state->ang_vel / f_state->G, lp_state->lambda);
		else
			fprintf(output_params->output_file, "%.14g\t%.14g\t%.14g\t%.14g\n", (double)it * f_state->G, p_state->angle, p_state->ang_vel / f_state->G, 0.0);
		fflush(output_params->output_file);
	}

	if(output_params->print_rho) {
		sprintf(file_name, "%s/%s%d.txt", output_params->output_folder, "rho", it);
		write_array_to_new_file(file_name, f_state->lx, f_state->ly, f_state->rho);
	}

	if(output_params->print_ux) {
		sprintf(file_name, "%s/%s%d.txt", output_params->output_folder, "ux", it);
		write_array_to_new_file(file_name, f_state->lx, f_state->ly, f_state->u[0]);
	}

	if(output_params->print_uy) {
		sprintf(file_name, "%s/%s%d.txt", output_params->output_folder, "uy", it);
		write_array_to_new_file(file_name, f_state->lx, f_state->ly, f_state->u[1]);
	}
}

void write_array_to_new_file(char * file_name, unsigned int lx, unsigned int ly, double * arr)
{
	unsigned int i, j;
	FILE * handle = fopen(file_name, "w");

	for(i = 0; i < lx; ++i) {
		for(j = 0; j < ly; ++j) {
			fprintf(handle, "%d\t%d\t%.14g\n", i, j, arr[i*ly+j]);
		}
	}
	fclose(handle);
}
