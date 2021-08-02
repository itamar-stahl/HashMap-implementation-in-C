#include "Vector.h"

#define FAIL -1
#define SUCCESS 1
#define NO_SUCCESS 0
#define ADD 1
#define REMOVE -1
#define ALLOC_CHECK_VALIDITY(elem_copy_func, elem_cmp_func, elem_free_func) \
           if (!elem_copy_func||!elem_cmp_func||!elem_free_func) {return NULL;}
#define FREE_CHECK_VALIDITY(p_vector) \
                              if (!p_vector||!(*p_vector)) {return;}
#define VECTOR_AT_CHECK_VALIDITY(vector, ind) \
   if (!vector||!(vector->data)||(ind >= (vector->size))) {return NULL;}
#define PUSH_CHECK_VALIDITY(vector, value) \
                  if (!vector||!(vector->data)||!value) {return NO_SUCCESS;}
#define FIND_CHECK_VALIDITY(vector, value) \
                  if (!vector||!(vector->data)||!value) {return FAIL;}
#define ERASE_CHECK_VALIDITY(vector, ind) \
  if (!vector||!(vector->data)||(ind >= vector->size)) {return NO_SUCCESS;}
#define RESIZE_CHECK_VALIDITY(vector) \
  if (!vector||!(vector->data)) {return;}

/*
 * Resize the vector size when it's necessary. See the Header file's documentation.
 * on VECTOR_GROWTH_FACTOR, VECTOR_MAX_LOAD_FACTOR, VECTOR_MIN_LOAD_FACTOR
 */
void ResizeVector(Vector *vector, int change){
  RESIZE_CHECK_VALIDITY(vector)
  double load_factor = VectorGetLoadFactor(vector);
  if ((load_factor > VECTOR_MAX_LOAD_FACTOR)&&(change==ADD)){
    vector->capacity *= VECTOR_GROWTH_FACTOR;
  } else if ((load_factor < VECTOR_MIN_LOAD_FACTOR)&&(change==REMOVE)) {
    vector->capacity /= VECTOR_GROWTH_FACTOR;
  } else return;
  vector->data = realloc(vector->data, vector->capacity*sizeof(void *));
}

/**
 * Allocates dynamically a new vector element.
 * @param elem_copy_func func which copies the element stored in the vector (returns
 * dynamically allocated copy).
 * @param elem_cmp_func func which is used to compare elements stored in the vector.
 * @param elem_free_func func which frees elements stored in the vector.
 * @return pointer to dynamically allocated vector.
 * @if_fail return NULL.
 */
Vector *VectorAlloc(VectorElemCpy elem_copy_func, VectorElemCmp elem_cmp_func,
                                                VectorElemFree elem_free_func){
  ALLOC_CHECK_VALIDITY(elem_copy_func, elem_cmp_func, elem_free_func)
  Vector *new_vec = malloc(sizeof(Vector));
  if (new_vec){
    new_vec->capacity = VECTOR_INITIAL_CAP;
    new_vec->size = 0;
    new_vec->elem_free_func = elem_free_func;
    new_vec->elem_cmp_func = elem_cmp_func;
    new_vec->elem_copy_func = elem_copy_func;
    new_vec->data = malloc(VECTOR_INITIAL_CAP*sizeof(void *));
    if (!new_vec->data){
      free(new_vec);
      new_vec = NULL;
    }
  }
  return new_vec;
}

/**
 * Frees a vector and the elements the vector itself allocated.
 * @param p_vector pointer to dynamically allocated pointer to vector.
 */
void VectorFree(Vector **p_vector){
  FREE_CHECK_VALIDITY(p_vector)
    if ((*p_vector)->data) {
      for (size_t i=0; i < (*p_vector)->size; i++) {
        if ((*p_vector)->data[i]){
          (*p_vector)->elem_free_func(&((*p_vector)->data[i]));
        }
      }
      free((*p_vector)->data);
    }
    free(*p_vector);
    *p_vector = NULL;
}

/**
 * Returns the element at the given index.
 * @param vector pointer to a vector.
 * @param ind the index of the element we want to get.
 * @return the element the given index if exists (the element itself, not a copy of it)
 * , NULL otherwise.
 */
void *VectorAt(Vector *vector, size_t ind){
  VECTOR_AT_CHECK_VALIDITY(vector, ind)
  return vector->data[ind];
}

/**
 * Gets a value and checks if the value is in the vector.
 * @param vector a pointer to vector.
 * @param value the value to look for.
 * @return the index of the given value if it is in the
 * vector ([0, vector_size - 1]).
 * Returns -1 if no such value in the vector.
 */
int VectorFind(Vector *vector, void *value){
  FIND_CHECK_VALIDITY(vector, value)
  for (size_t i=0; i < vector->size; i++){
    if(!vector->data[i]) continue;
    if (vector->elem_cmp_func(vector->data[i], value)) {return (int) i;}
  }
  return FAIL;
}

/**
 * Adds a new value to the back (index vector_size) of the vector.
 * @param vector a pointer to vector.
 * @param value the value to be added to the vector.
 * @return 1 if the adding has been done successfully, 0 otherwise.
 */
int VectorPushBack(Vector *vector, void *value){
  PUSH_CHECK_VALIDITY(vector, value)
  vector->data[(vector->size)++] = vector->elem_copy_func(value);
  if (vector->data[vector->size-1]){
    ResizeVector(vector, ADD);
    return SUCCESS;
  }
  (vector->size)--;
  return NO_SUCCESS;
}


/**
 * This function returns the load factor of the vector.
 * @param vector a vector.
 * @return the vector's load factor, -1 if the function failed.
 */
double VectorGetLoadFactor(Vector *vector){
  if (vector&&vector->capacity){
    return (((double)vector->size)/vector->capacity);
  }
  return (double) FAIL;
}

/*
 * Arrange the Vector after removing an element (a helper func to VectorErase)
 */
void ArrangeVector (Vector *vector, int hole_idx){
  for (size_t i = hole_idx; i < vector->size; i++){
    vector->data[i] = vector->data[i+1];
  }
}

/**
 * Removes the element at the given index from the vector.
 * @param vector a pointer to vector.
 * @param ind the index of the element to be removed.
 * @return 1 if the removing has been done successfully, 0 otherwise.
 */
int VectorErase(Vector *vector, size_t ind){
  ERASE_CHECK_VALIDITY(vector, ind)
  vector->elem_free_func(&(vector->data[ind]));
  ArrangeVector(vector, ind);
  vector->size--;
  ResizeVector(vector, REMOVE);
  return SUCCESS;
}

/**
 * Deletes all the elements in the vector.
 * @param vector vector a pointer to vector.
 */
void VectorClear(Vector *vector){
  if(vector&&(vector->data)){
    for (size_t i = (vector->size); i > 0; --i){
      VectorErase(vector, i-1);
    }
  }
}