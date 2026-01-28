#include "ctx.h"
#include "model.h"
#include "policy.h"

#include <span>

extern "C" {
#define PY_SSIZE_T_CLEAN
#include <Python.h>

static PyObject *
predict( PyObject *self, PyObject *args )
{
        (void)( self );
        (void)( args );
        int boards[BOARD_SIZE] = { };
        int predict_col = c4::policy_call( std::span{ boards }, BLACK_INT );
        DEBUG( ) << "predict col is " << predict_col << "\n";
        c4::model_deinit( );
        return PyLong_FromLong( 0 );
}

static PyMethodDef Methods[] = {
    {"predict", predict, METH_VARARGS, "Execute a shell command."},
    {     NULL,    NULL,            0,                       NULL}  /* Sentinel */
};

static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT, "c4_sys", /* name of module */
    NULL,                            /* module documentation, may be NULL */
    -1, /* size of per-interpreter state of the module,
           or -1 if the module keeps state in global variables. */
    Methods };

PyMODINIT_FUNC
PyInit_c4_sys( void )
{
        return PyModule_Create( &module );
}
}
