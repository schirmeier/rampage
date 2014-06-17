#include <Python.h>
#include <stdio.h>
#include <string.h>

// invocation from Python:  ret = memtestcext.lineartest(region, offset, len)
static PyObject *
memtestcext_lineartest(PyObject *self, PyObject *args)
{
	unsigned offset, len;
	unsigned char *region, *p;

	// FIXME: offset and len could be longer than "unsigned int"
	if (!PyArg_ParseTuple(args, "wII", &region, &offset, &len))
		return NULL;

//	printf("CEXT HERE, %p %u %u\n", region, offset, len);

	// test all 0s
	memset(region + offset, 0, len);
	for (p = region + offset; p < region + offset + len; ++p) {
		if (*p != 0) {
			return Py_BuildValue("i", p - region - offset);
		}
	}

	// test all 1s
	memset(region + offset, 0xff, len);
	for (p = region + offset; p < region + offset + len; ++p) {
		if (*p != 0xff) {
			return Py_BuildValue("i", p - region - offset);
		}
	}

	// test all ADDR
	for (p = region + offset; p < region + offset + len; ++p) {
		*p = ((uintptr_t) p) & 0xff;
	}
	for (p = region + offset; p < region + offset + len; ++p) {
		if (*p != (((uintptr_t) p) & 0xff)) {
			return Py_BuildValue("i", p - region - offset);
		}
	}

	return Py_BuildValue("i", -1);
}

static PyMethodDef memtestcext_methods[] = {
	{"lineartest",  memtestcext_lineartest, METH_VARARGS,
		"linear-time memory testing"},
	{NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC
initmemtestcext(void)
{
	(void) Py_InitModule("memtestcext", memtestcext_methods);
}
