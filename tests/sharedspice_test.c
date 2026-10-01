#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "ngspice/sharedspice.h"

static int ng_getchar(char *outputreturn, int ident, void *userdata) {
    (void)ident; (void)userdata;
    printf("[ngspice stdout] %s\n", outputreturn);
    return 0;
}

static int ng_getstat(char *outputreturn, int ident, void *userdata) {
    (void)ident; (void)userdata; (void)outputreturn;
    return 0;
}

static int ng_exit(int exitstatus, NG_BOOL immediate, NG_BOOL quitexit, int ident, void *userdata) {
    (void)exitstatus; (void)immediate; (void)quitexit; (void)ident; (void)userdata;
    return 0;
}

static int ng_data(pvecvaluesall vdata, int numvecs, int ident, void *userdata) {
    (void)vdata; (void)numvecs; (void)ident; (void)userdata;
    return 0;
}

static int ng_initdata(pvecinfoall idata, int ident, void *userdata) {
    (void)idata; (void)ident; (void)userdata;
    return 0;
}

static int ng_thread_runs(NG_BOOL norun, int ident, void *userdata) {
    (void)norun; (void)ident; (void)userdata;
    return 0;
}

int main(void) {
    printf("Initializing ngspice shared library...\n");
    ngSpice_Init(ng_getchar, ng_getstat, ng_exit, ng_data, ng_initdata, ng_thread_runs, NULL);

    char *circuit[] = {
        "Test Circuit - Voltage Divider",
        "V1 in 0 DC 10",
        "R1 in out 1k",
        "R2 out 0 1k",
        ".op",
        ".end",
        NULL,
    };

    printf("Loading circuit with ngSpice_Circ...\n");
    int ret = ngSpice_Circ(circuit);
    if (ret != 0) {
        fprintf(stderr, "ngSpice_Circ failed with code %d\n", ret);
        return 1;
    }

    printf("Running command 'op'...\n");
    ret = ngSpice_Command("op");
    if (ret != 0) {
        fprintf(stderr, "ngSpice_Command('op') failed with code %d\n", ret);
        return 1;
    }

    printf("Retrieving output vectors...\n");
    pvector_info vec_out = ngGet_Vec_Info("V(out)");
    if (!vec_out) {
        vec_out = ngGet_Vec_Info("out");
    }
    if (!vec_out) {
        fprintf(stderr, "Error: vector 'out' / 'V(out)' not found\n");
        return 1;
    }

    if (vec_out->v_length < 1 || !vec_out->v_realdata) {
        fprintf(stderr, "Error: vector 'out' has invalid length or data\n");
        return 1;
    }

    double val_out = vec_out->v_realdata[0];
    printf("V(out) = %f (expected 5.0)\n", val_out);
    if (fabs(val_out - 5.0) > 1e-3) {
        fprintf(stderr, "Error: V(out) %f != expected 5.0\n", val_out);
        return 1;
    }

    pvector_info vec_in = ngGet_Vec_Info("V(in)");
    if (!vec_in) {
        vec_in = ngGet_Vec_Info("in");
    }
    if (!vec_in) {
        fprintf(stderr, "Error: vector 'in' / 'V(in)' not found\n");
        return 1;
    }

    double val_in = vec_in->v_realdata[0];
    printf("V(in) = %f (expected 10.0)\n", val_in);
    if (fabs(val_in - 10.0) > 1e-3) {
        fprintf(stderr, "Error: V(in) %f != expected 10.0\n", val_in);
        return 1;
    }

    printf("SUCCESS: sharedspice_test passed!\n");
    return 0;
}
