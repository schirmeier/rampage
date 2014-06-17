#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "algo.h"
#include "error.h"

static PyObject* convert_errorlist(struct list_head *errors) {
        struct list_head *cur   = NULL;
        struct mem_error *error = NULL;
        PyObject *list          = PyList_New(0);

        if (list == NULL)
                abort();

        cur = errors;
        do {
                if (cur->content != NULL) {
                        error = (struct mem_error *)cur->content;

                        if (PyList_Append(list, PyInt_FromSize_t(error->offset))) {
                                // exception
                                abort();
                        }
                }
                cur = cur->next;
        } while (cur != errors);

        return list;
}

static int parse_args(PyObject *args, def_int_t **start, def_int_t **end) {
        unsigned offset, len;
        def_int_t *region;

        if (!PyArg_ParseTuple(args, "wII", &region, &offset, &len))
                return 1;

        *start = region + (offset / sizeof(def_int_t));
        *end   = *start + len / sizeof(def_int_t);

        return 0;
}

static PyObject* mt86_address_bits(PyObject *self, PyObject *args) {
        def_int_t *start, *end;
        struct list_head *errors = NULL;
        PyObject *result;

        if (parse_args(args, &start, &end))
                return NULL;

	if (end - start < 0x2000000)
	{
                result = PyList_New(0); // FIXME: can return NULL
                Py_INCREF(result);
		printf("ERROR: Test window is too small (size does matter :-D)\n");
                return result; // return empty error list
	}

	errors = test_address_bits(start,end,0x0);
	if (errors != NULL)
	{
                result = convert_errorlist(errors);
		free_error_list(errors);
	} else {
                result = PyList_New(0);
        }

        Py_INCREF(result);
        return result;
}

static PyObject* mt86_address_own(PyObject *self, PyObject *args) {
        def_int_t *start, *end;
        struct list_head *errors = NULL;
        PyObject *result;

        if (parse_args(args, &start, &end))
                return NULL;

	errors = test_address_own(start,end);
	if (errors != NULL)
	{
                PyObject *result = convert_errorlist(errors);
		free_error_list(errors);
		return result;
	} else {
                result = PyList_New(0);
        }

        Py_INCREF(result);
        return result;
}

static PyObject* mt86_mv_inv_onezeros(PyObject *self, PyObject *args) {
        def_int_t *start, *end;
        struct list_head *errors = NULL;
        PyObject *result;

        if (parse_args(args, &start, &end))
                return NULL;

        errors = test_mv_inv_onezeros(start,end);
	if (errors != NULL)
	{
                PyObject *result = convert_errorlist(errors);
		free_error_list(errors);
		return result;
	} else {
                result = PyList_New(0);
        }

        Py_INCREF(result);
        return result;
}

static PyObject* mt86_mv_inv_8bit(PyObject *self, PyObject *args) {
        def_int_t *start, *end;
        struct list_head *errors = NULL;
        PyObject *result;

        if (parse_args(args, &start, &end))
                return NULL;

	errors = test_mv_inv_eightpat(start,end);
	if (errors != NULL)
	{
                PyObject *result = convert_errorlist(errors);
		free_error_list(errors);
		return result;
	} else {
                result = PyList_New(0);
        }

        Py_INCREF(result);
        return result;
}

static PyObject* mt86_mv_inv_rand(PyObject *self, PyObject *args) {
        def_int_t *start, *end;
        struct list_head *errors = NULL;
        PyObject *result;

        if (parse_args(args, &start, &end))
                return NULL;

	errors = test_mv_inv_rand(start,end);
	if (errors != NULL)
	{
                PyObject *result = convert_errorlist(errors);
		free_error_list(errors);
		return result;
	} else {
                result = PyList_New(0);
        }

        Py_INCREF(result);
        return result;
}

static PyObject* mt86_blockmove(PyObject *self, PyObject *args) {
        def_int_t *start, *end;
        struct list_head *errors = NULL;
        PyObject *result;

        if (parse_args(args, &start, &end))
                return NULL;

        errors = test_block_move(start,end);
        if (errors != NULL) {
                PyObject *result = convert_errorlist(errors);
                free_error_list(errors);
                return result;
        } else {
                result = PyList_New(0);
        }

        Py_INCREF(result);
        return result;
}

static PyObject *mt86_mv_inv_32bit(PyObject *self, PyObject *args) {
        def_int_t *start, *end;
        struct list_head *errors = NULL;
        PyObject *result;

        if (parse_args(args, &start, &end))
                return NULL;

	errors = test_mv_inv_32(start,end);
	if (errors != NULL)
	{
                PyObject *result = convert_errorlist(errors);
		free_error_list(errors);
		return result;
	} else {
                result = PyList_New(0);
        }

        Py_INCREF(result);
        return result;
}

static PyObject *mt86_random_numbers(PyObject *self, PyObject *args) {
        def_int_t *start, *end;
        struct list_head *errors = NULL;
        PyObject *result;

        if (parse_args(args, &start, &end))
                return NULL;

	errors = test_random_numbers(start,end);
	if (errors != NULL)
	{
                PyObject *result = convert_errorlist(errors);
		free_error_list(errors);
		return result;
	} else {
                result = PyList_New(0);
        }

        Py_INCREF(result);
        return result;
}

static PyObject *mt86_mod20(PyObject *self, PyObject *args) {
        def_int_t *start, *end;
        struct list_head *errors = NULL;
        PyObject *result;

        if (parse_args(args, &start, &end))
                return NULL;

	errors = test_mod_20(start,end);
	if (errors != NULL)
	{
                PyObject *result = convert_errorlist(errors);
		free_error_list(errors);
		return result;
	} else {
                result = PyList_New(0);
        }

        Py_INCREF(result);
        return result;
}

static PyMethodDef memtest86pcext_methods[] = {
        {"address_bits",    mt86_address_bits,    METH_VARARGS, "Address test, walking ones"        },
        {"address_own",     mt86_address_own,     METH_VARARGS, "Address test, own address"         },
        {"mv_inv_onezeros", mt86_mv_inv_onezeros, METH_VARARGS, "Moving inversions, ones & zeros"   },
        {"mv_inv_8bit",     mt86_mv_inv_8bit,     METH_VARARGS, "Moving inversions, 8 bit pattern"  },
        {"mv_inv_rand",     mt86_mv_inv_rand,     METH_VARARGS, "Moving inversions, random pattern" },
        {"blockmove",       mt86_blockmove,       METH_VARARGS, "Block move, 80 moves"              },
        {"mv_inv_32bit",    mt86_mv_inv_32bit,    METH_VARARGS, "Moving inversions, 32 bit pattern" },
        {"random_numbers",  mt86_random_numbers,  METH_VARARGS, "Random number sequence"            },
        {"mod20",           mt86_mod20,           METH_VARARGS, "Modulo 20, Random pattern"         },

	{NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC
initmemtest86pcext(void)
{
	(void) Py_InitModule("memtest86pcext", memtest86pcext_methods);
}
