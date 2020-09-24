/*
 * Written in 2013 by Dmitry Chestnykh <dmitry@codingrobots.com>
 * Modified for CPython by Christian Heimes <christian@python.org>
 *
 * To the extent possible under law, the author have dedicated all
 * copyright and related and neighboring rights to this software to
 * the public domain worldwide. This software is distributed without
 * any warranty. http://creativecommons.org/publicdomain/zero/1.0/
 */

/* WARNING: autogenerated file!
 *
 * The blake2s_impl.c is autogenerated from blake2s_impl.c.
 */

#include "Python.h"
#include "pystrhex.h"

#include "../hashlib.h"
#include "blake2ns.h"

#define HAVE_BLAKE2S 1
#define BLAKE2_LOCAL_INLINE(type) Py_LOCAL_INLINE(type)

#include "impl/blake2.h"
#include "impl/blake2-impl.h" /* for secure_zero_memory() and store48() */

/* pure SSE2 implementation is very slow, so only use the more optimized SSSE3+
 * https://bugs.python.org/issue31834 */
#if defined(__SSSE3__) || defined(__SSE4_1__) || defined(__AVX__) || defined(__XOP__)
#include "impl/blake2s.c"
#else
#include "impl/blake2s-ref.c"
#endif

extern PyType_Spec blake2s_type_spec;

typedef struct {
    PyObject_HEAD
    blake2s_param    param;
    blake2s_state    state;
    PyThread_type_lock lock;
} BLAKE2sObject;

#include "clinic/blake2s_impl.c.h"

/*[clinic input]
module _blake2
class _blake2.blake2s "BLAKE2sObject *" "&PyBlake2_BLAKE2sType"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=4b79d7ffe07286ce]*/


static BLAKE2sObject *
new_BLAKE2sObject(PyTypeObject *type)
{
    BLAKE2sObject *self;
    self = (BLAKE2sObject *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->lock = NULL;
    }
    return self;
}

/*[clinic input]
@classmethod
_blake2.blake2s.__new__ as py_blake2s_new
    data: object(c_default="NULL") = b''
    /
    *
    digest_size: int(c_default="BLAKE2S_OUTBYTES") = _blake2.blake2s.MAX_DIGEST_SIZE
    key: Py_buffer(c_default="NULL", py_default="b''") = None
    salt: Py_buffer(c_default="NULL", py_default="b''") = None
    person: Py_buffer(c_default="NULL", py_default="b''") = None
    fanout: int = 1
    depth: int = 1
    leaf_size: unsigned_long = 0
    node_offset: unsigned_long_long = 0
    node_depth: int = 0
    inner_size: int = 0
    last_node: bool = False
    usedforsecurity: bool = True

Return a new BLAKE2s hash object.
[clinic start generated code]*/

static PyObject *
py_blake2s_new_impl(PyTypeObject *type, PyObject *data, int digest_size,
                    Py_buffer *key, Py_buffer *salt, Py_buffer *person,
                    int fanout, int depth, unsigned long leaf_size,
                    unsigned long long node_offset, int node_depth,
                    int inner_size, int last_node, int usedforsecurity)
/*[clinic end generated code: output=556181f73905c686 input=4dda87723f23abb0]*/
{
    BLAKE2sObject *self = NULL;
    Py_buffer buf;

    self = new_BLAKE2sObject(type);
    if (self == NULL) {
        goto error;
    }

    /* Zero parameter block. */
    memset(&self->param, 0, sizeof(self->param));

    /* Set digest size. */
    if (digest_size <= 0 || digest_size > BLAKE2S_OUTBYTES) {
        PyErr_Format(PyExc_ValueError,
                "digest_size must be between 1 and %d bytes",
                BLAKE2S_OUTBYTES);
        goto error;
    }
    self->param.digest_length = digest_size;

    /* Set salt parameter. */
    if ((salt->obj != NULL) && salt->len) {
        if (salt->len > BLAKE2S_SALTBYTES) {
            PyErr_Format(PyExc_ValueError,
                "maximum salt length is %d bytes",
                BLAKE2S_SALTBYTES);
            goto error;
        }
        memcpy(self->param.salt, salt->buf, salt->len);
    }

    /* Set personalization parameter. */
    if ((person->obj != NULL) && person->len) {
        if (person->len > BLAKE2S_PERSONALBYTES) {
            PyErr_Format(PyExc_ValueError,
                "maximum person length is %d bytes",
                BLAKE2S_PERSONALBYTES);
            goto error;
        }
        memcpy(self->param.personal, person->buf, person->len);
    }

    /* Set tree parameters. */
    if (fanout < 0 || fanout > 255) {
        PyErr_SetString(PyExc_ValueError,
                "fanout must be between 0 and 255");
        goto error;
    }
    self->param.fanout = (uint8_t)fanout;

    if (depth <= 0 || depth > 255) {
        PyErr_SetString(PyExc_ValueError,
                "depth must be between 1 and 255");
        goto error;
    }
    self->param.depth = (uint8_t)depth;

    if (leaf_size > 0xFFFFFFFFU) {
        PyErr_SetString(PyExc_OverflowError, "leaf_size is too large");
        goto error;
    }
    // NB: Simple assignment here would be incorrect on big endian platforms.
    store32(&(self->param.leaf_length), leaf_size);

#ifdef HAVE_BLAKE2S
    if (node_offset > 0xFFFFFFFFFFFFULL) {
        /* maximum 2**48 - 1 */
         PyErr_SetString(PyExc_OverflowError, "node_offset is too large");
         goto error;
     }
    store48(&(self->param.node_offset), node_offset);
#else
    // NB: Simple assignment here would be incorrect on big endian platforms.
    store64(&(self->param.node_offset), node_offset);
#endif

    if (node_depth < 0 || node_depth > 255) {
        PyErr_SetString(PyExc_ValueError,
                "node_depth must be between 0 and 255");
        goto error;
    }
    self->param.node_depth = node_depth;

    if (inner_size < 0 || inner_size > BLAKE2S_OUTBYTES) {
        PyErr_Format(PyExc_ValueError,
                "inner_size must be between 0 and is %d",
                BLAKE2S_OUTBYTES);
        goto error;
    }
    self->param.inner_length = inner_size;

    /* Set key length. */
    if ((key->obj != NULL) && key->len) {
        if (key->len > BLAKE2S_KEYBYTES) {
            PyErr_Format(PyExc_ValueError,
                "maximum key length is %d bytes",
                BLAKE2S_KEYBYTES);
            goto error;
        }
        self->param.key_length = (uint8_t)key->len;
    }

    /* Initialize hash state. */
    if (blake2s_init_param(&self->state, &self->param) < 0) {
        PyErr_SetString(PyExc_RuntimeError,
                "error initializing hash state");
        goto error;
    }

    /* Set last node flag (must come after initialization). */
    self->state.last_node = last_node;

    /* Process key block if any. */
    if (self->param.key_length) {
        uint8_t block[BLAKE2S_BLOCKBYTES];
        memset(block, 0, sizeof(block));
        memcpy(block, key->buf, key->len);
        blake2s_update(&self->state, block, sizeof(block));
        secure_zero_memory(block, sizeof(block));
    }

    /* Process initial data if any. */
    if (data != NULL) {
        GET_BUFFER_VIEW_OR_ERROR(data, &buf, goto error);

        if (buf.len >= HASHLIB_GIL_MINSIZE) {
            Py_BEGIN_ALLOW_THREADS
            blake2s_update(&self->state, buf.buf, buf.len);
            Py_END_ALLOW_THREADS
        } else {
            blake2s_update(&self->state, buf.buf, buf.len);
        }
        PyBuffer_Release(&buf);
    }

    return (PyObject *)self;

  error:
    if (self != NULL) {
        Py_DECREF(self);
    }
    return NULL;
}

/*[clinic input]
_blake2.blake2s.copy

Return a copy of the hash object.
[clinic start generated code]*/

static PyObject *
_blake2_blake2s_copy_impl(BLAKE2sObject *self)
/*[clinic end generated code: output=5b90131c4eae275e input=0b9d44942f0fe4b2]*/
{
    BLAKE2sObject *cpy;

    if ((cpy = new_BLAKE2sObject(Py_TYPE(self))) == NULL)
        return NULL;

    ENTER_HASHLIB(self);
    cpy->param = self->param;
    cpy->state = self->state;
    LEAVE_HASHLIB(self);
    return (PyObject *)cpy;
}

/*[clinic input]
_blake2.blake2s.update

    data: object
    /

Update this hash object's state with the provided bytes-like object.
[clinic start generated code]*/

static PyObject *
_blake2_blake2s_update(BLAKE2sObject *self, PyObject *data)
/*[clinic end generated code: output=757dc087fec37815 input=97500db2f9de4aaa]*/
{
    Py_buffer buf;

    GET_BUFFER_VIEW_OR_ERROUT(data, &buf);

    if (self->lock == NULL && buf.len >= HASHLIB_GIL_MINSIZE)
        self->lock = PyThread_allocate_lock();

    if (self->lock != NULL) {
       Py_BEGIN_ALLOW_THREADS
       PyThread_acquire_lock(self->lock, 1);
       blake2s_update(&self->state, buf.buf, buf.len);
       PyThread_release_lock(self->lock);
       Py_END_ALLOW_THREADS
    } else {
        blake2s_update(&self->state, buf.buf, buf.len);
    }
    PyBuffer_Release(&buf);

    Py_RETURN_NONE;
}

/*[clinic input]
_blake2.blake2s.digest

Return the digest value as a bytes object.
[clinic start generated code]*/

static PyObject *
_blake2_blake2s_digest_impl(BLAKE2sObject *self)
/*[clinic end generated code: output=40c566ca4bc6bc51 input=f41e0b8d6d937454]*/
{
    uint8_t digest[BLAKE2S_OUTBYTES];
    blake2s_state state_cpy;

    ENTER_HASHLIB(self);
    state_cpy = self->state;
    blake2s_final(&state_cpy, digest, self->param.digest_length);
    LEAVE_HASHLIB(self);
    return PyBytes_FromStringAndSize((const char *)digest,
            self->param.digest_length);
}

/*[clinic input]
_blake2.blake2s.hexdigest

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
_blake2_blake2s_hexdigest_impl(BLAKE2sObject *self)
/*[clinic end generated code: output=15153eb5e59c52eb input=c77a1321567e8952]*/
{
    uint8_t digest[BLAKE2S_OUTBYTES];
    blake2s_state state_cpy;

    ENTER_HASHLIB(self);
    state_cpy = self->state;
    blake2s_final(&state_cpy, digest, self->param.digest_length);
    LEAVE_HASHLIB(self);
    return _Py_strhex((const char *)digest, self->param.digest_length);
}


static PyMethodDef py_blake2s_methods[] = {
    _BLAKE2_BLAKE2S_COPY_METHODDEF
    _BLAKE2_BLAKE2S_DIGEST_METHODDEF
    _BLAKE2_BLAKE2S_HEXDIGEST_METHODDEF
    _BLAKE2_BLAKE2S_UPDATE_METHODDEF
    {NULL, NULL}
};



static PyObject *
py_blake2s_get_name(BLAKE2sObject *self, void *closure)
{
    return PyUnicode_FromString("blake2s");
}



static PyObject *
py_blake2s_get_block_size(BLAKE2sObject *self, void *closure)
{
    return PyLong_FromLong(BLAKE2S_BLOCKBYTES);
}



static PyObject *
py_blake2s_get_digest_size(BLAKE2sObject *self, void *closure)
{
    return PyLong_FromLong(self->param.digest_length);
}


static PyGetSetDef py_blake2s_getsetters[] = {
    {"name", (getter)py_blake2s_get_name,
        NULL, NULL, NULL},
    {"block_size", (getter)py_blake2s_get_block_size,
        NULL, NULL, NULL},
    {"digest_size", (getter)py_blake2s_get_digest_size,
        NULL, NULL, NULL},
    {NULL}
};


static void
py_blake2s_dealloc(PyObject *self)
{
    BLAKE2sObject *obj = (BLAKE2sObject *)self;

    /* Try not to leave state in memory. */
    secure_zero_memory(&obj->param, sizeof(obj->param));
    secure_zero_memory(&obj->state, sizeof(obj->state));
    if (obj->lock) {
        PyThread_free_lock(obj->lock);
        obj->lock = NULL;
    }

    PyTypeObject *type = Py_TYPE(self);
    PyObject_Del(self);
    Py_DECREF(type);
}

static PyType_Slot blake2s_type_slots[] = {
    {Py_tp_dealloc, py_blake2s_dealloc},
    {Py_tp_doc, (char *)py_blake2s_new__doc__},
    {Py_tp_methods, py_blake2s_methods},
    {Py_tp_getset, py_blake2s_getsetters},
    {Py_tp_new, py_blake2s_new},
    {0,0}
};

PyType_Spec blake2s_type_spec = {
    .name = "_blake2.blake2s",
    .basicsize =  sizeof(BLAKE2sObject),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = blake2s_type_slots
};
