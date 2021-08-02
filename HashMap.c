#include "HashMap.h"

#define SUCCESS 1
#define FAILURE 0
#define FAIL (-1)
#define KEY 1
#define VALUE 2
#define NOT_FOUND 0
#define ADD 1
#define REMOVE -1
#define INSTALL_WRAPPER(cmp_func_1, cmp_func_2) \
      ElemCmpWrapper(cmp_func_1, cmp_func_2);  \
      vector->elem_cmp_func = &ElemCmpWrapper;
#define ALLOC_CHECK_VALIDITY(hash_func, pair_cpy, pair_cmp, pair_free) \
            if (!(hash_func&&pair_cpy&&pair_cmp&&pair_free)) {return NULL;}
#define FREE_CHECK_VALIDITY(map) if(!map||!(*map)) {return;}
#define CONTAINS_CHECK_VALIDITY(map, key_or_val) \
   if(!map||!(map->buckets)||(map->size == 0)||!key_or_val) {return NOT_FOUND;}
#define FREE_VECTORS(start, end) \
      for (size_t j = start; j < end; j++) {VectorFree(&(map->buckets[j]));}
#define ERASE_CHECK_VALIDITY(hash_map, key) \
  if (!hash_map||!(hash_map->buckets)||(hash_map->size == 0)||!key) \
                                                      {return NOT_FOUND;}
#define RESIZE_CHECK_VALIDITY(map) \
  if (!map||!(map->buckets)) {return;}
#define INSERT_CHECK_VALIDITY(map, pair) \
      if (!map||!map->buckets||!pair) {return FAILURE;}

/*
 * Returns the index of bucket for a given map and key
 */
size_t Hash(HashMap *map, size_t capacity, KeyT key){
  return (map->hash_func(key) & (capacity - 1));
}

/*
* Realloc map->buckets and dynamically allocates vectors into array in the range
 * [map->capacity, new_capacity]. Returns 1 when it Successes. 0 when it fails.
*/
int VectorsAllocator(HashMap *map, size_t new_capacity){
  Vector **tmp = realloc(map->buckets, sizeof(void *)*new_capacity);
  if (tmp) {
    map->buckets = tmp;
  } else return FAILURE;

  for (size_t i = map->capacity; i < new_capacity; i++){
    map->buckets[i] = VectorAlloc(map->pair_cpy, map->pair_cmp, map->pair_free);
    if (!map->buckets[i]) {
      FREE_VECTORS(map->capacity, i)
      map->buckets = realloc(map->buckets, sizeof(void *)*(map->capacity));
      return FAILURE;
    }
  }
  map->capacity = new_capacity;
  return SUCCESS;
}

/*
 * A helper function to Re-inserts
 */
void ReInsertBucket(HashMap *map, size_t bucket_idx,
                                    size_t old_capacity, size_t new_capacity){
  Vector *vector;
  if ((!(vector = map->buckets[bucket_idx]))||(vector->size == 0)) {return;}
  for (size_t j = vector->size; j > 0; j--){
    Pair *pair = vector->data[j-1];
    if ((old_capacity < new_capacity) &&
       (Hash(map, new_capacity, pair->key) ==
                        Hash(map, old_capacity, pair->key))) {return;}
    HashMapInsert(map, pair);
    VectorErase(vector, j-1);
    map->size--;
  }
}

/*
 * Re-inserts elements after resize
 */
void ReInsertElems(HashMap *map, size_t old_capacity, size_t new_capacity){
  if (old_capacity > new_capacity){ // decrease
    for (size_t i = new_capacity; i < old_capacity; i++) {
      ReInsertBucket(map, i, old_capacity, new_capacity);
    }
  } else if (old_capacity < new_capacity){ //increase
    for (size_t i = 0; i < old_capacity; i++) {
      ReInsertBucket(map, i, old_capacity, new_capacity);
    }
  }
}

/*
 * Resize the map size when it's necessary. See Header file's documentation
 * on HASH_MAP_GROWTH_FACTOR, HASH_MAP_MAX_LOAD_FACTOR, HASH_MAP_MIN_LOAD_FACTOR
 */
void ResizeMap(HashMap *map, int change){
  RESIZE_CHECK_VALIDITY(map)
  double load_factor = HashMapGetLoadFactor(map);
  size_t new_cap, old_cap; new_cap = (old_cap = map->capacity);
  if ((load_factor > HASH_MAP_MAX_LOAD_FACTOR)&&(change==ADD)){
    new_cap *= HASH_MAP_GROWTH_FACTOR;
    if (!VectorsAllocator(map, new_cap)) {return;}
    ReInsertElems(map, old_cap, new_cap);
  } else if ((load_factor < HASH_MAP_MIN_LOAD_FACTOR)&&(change==REMOVE)) {
    new_cap /= HASH_MAP_GROWTH_FACTOR;
    map->capacity = new_cap;
    ReInsertElems(map, old_cap, new_cap);
    FREE_VECTORS(new_cap, old_cap)
    map->buckets = realloc(map->buckets, sizeof(void *)*new_cap);
  }
}

/*
 * A helper function for GetPairLocationByKey/ContainsValue functions.
 * Wraps the key/value compare function, for enabling using VectorFind() in
 * Vector.h module. The function has two modes:
 * 1) Installation mode, when GetPairLocationByKey/ContainsValue functions
 * set the corresponding elem_cmp_func into: static VectorElemCmp elem_cmp_func.
 * 2) Using mode, when VectorFind() can compare pairs by key/value.
 */
int ElemCmpWrapper(const void *elem_1, const void *elem_2) {
  static VectorElemCmp elem_cmp_func;
  static int key_or_value;
  if ((elem_1&&(!elem_2))) { //install key compare function
    elem_cmp_func = (VectorElemCmp) elem_1;
    key_or_value = KEY;
    return SUCCESS;
  } else if ((!elem_1)&&elem_2){ // install value compare function
    elem_cmp_func = (VectorElemCmp) elem_2;
    key_or_value = VALUE;
    return SUCCESS;
  }
  Pair *pair_1 = (Pair *) elem_1;
  return (key_or_value==KEY) ? (elem_cmp_func (pair_1->key, elem_2)) :
         (elem_cmp_func (pair_1->value, elem_2));
}

/*
 * Get HashMap and key. Set the bucket_idx in_vector_idx of the corresponding
 * pair if the key in the map. return 1 if the key exists, 0 otherwise
 */
int GetPairLocationByKey(KeyT key, HashMap *hash_map,
                         size_t *bucket_idx, size_t *in_vector_idx){
  *bucket_idx = Hash(hash_map, hash_map->capacity, key);
  Vector *vector = hash_map->buckets[*bucket_idx];
  if (vector->size == 0) return NOT_FOUND;
  // install wrapped key_cmp func in vector:
  INSTALL_WRAPPER(((Pair *) vector->data[0])->key_cmp , NULL)
  int return_val = VectorFind(vector, key);
  vector->elem_cmp_func = hash_map->pair_cmp; // restore pair cmp function
  *in_vector_idx = (size_t) return_val;
  return (return_val != FAIL);
}

/**
 * Allocates dynamically new hash map element.
 * @param hash_func a function which "hashes" keys.
 * @param pair_cpy a function which copies pairs.
 * @param pair_cmp a function which compares pairs.
 * @param pair_free a function which frees pairs.
 * @return pointer to dynamically allocated HashMap.
 * @if_fail return NULL.
 */
HashMap *HashMapAlloc(
    HashFunc hash_func, HashMapPairCpy pair_cpy,
    HashMapPairCmp pair_cmp, HashMapPairFree pair_free){
  ALLOC_CHECK_VALIDITY(hash_func, pair_cpy, pair_cmp, pair_free)
  HashMap *map = malloc(HASH_MAP_INITIAL_CAP*sizeof(void *));
  if (map) {
    map->size = 0;
    map->pair_free = pair_free;
    map->pair_cmp = pair_cmp;
    map->hash_func = hash_func;
    map->pair_cpy = pair_cpy;
    map->capacity = 0;
    map->buckets = NULL;
    if (!(VectorsAllocator(map, HASH_MAP_INITIAL_CAP))){
      free(map);
      map = NULL;
    }
  }
  return map;
}

/**
 * Frees a vector and the elements the vector itself allocated.
 * @param p_hash_map pointer to dynamically allocated pointer to hash_map.
 */
void HashMapFree(HashMap **p_hash_map){
  FREE_CHECK_VALIDITY(p_hash_map)
  HashMap *map = *p_hash_map;
  if (map->buckets){
    FREE_VECTORS(0, map->capacity)
    free(map->buckets);
    map->capacity = 0;
  }
  free(map);
  *p_hash_map = NULL;
}

/**
 * Inserts a new pair to the hash map.
 * The function inserts *new*, *copied*, *dynamically allocated* pair,
 * NOT the pair it receives as a parameter.
 * @param hash_map the hash map to be inserted with new element.
 * @param pair a pair the hash map would contain.
 * @return returns 1 for successful insertion, 0 otherwise.
 */
int HashMapInsert(HashMap *hash_map, Pair *pair){
  INSERT_CHECK_VALIDITY(hash_map, pair)
  KeyT key = pair->key;
  size_t bucket_idx, in_vector_idx;
  if (GetPairLocationByKey(key, hash_map, &bucket_idx, &in_vector_idx)){
    Vector *vector = hash_map->buckets[bucket_idx];
    vector->elem_free_func(&(vector->data[in_vector_idx]));
    vector->data[in_vector_idx] = vector->elem_copy_func(pair);
  } else {
    if (!VectorPushBack(hash_map->buckets[bucket_idx], pair)) {return FAILURE;}
    hash_map->size++;
    ResizeMap(hash_map, ADD);
  }
  return SUCCESS;
}

/**
 * The function checks if the given value exists in the hash map.
 * @param hash_map a hash map.
 * @param value the value to be checked.
 * @return 1 if the value is in the hash map, 0 otherwise.
 */
int HashMapContainsValue(HashMap *hash_map, ValueT value){
  CONTAINS_CHECK_VALIDITY(hash_map, value)
  Vector *vector;
  int return_val = NOT_FOUND;
  for (size_t i = 0; i < hash_map->capacity; i++) {
    vector = hash_map->buckets[i];
    if (vector->size == 0) continue;
    // install wrapped value_cmp func in vector:
    INSTALL_WRAPPER(NULL, ((Pair *) vector->data[0])->value_cmp)
    return_val = (VectorFind(vector, value) != FAIL);
    vector->elem_cmp_func = hash_map->pair_cmp; // restore pair cmp function
    if (return_val) break;
  }
  return return_val;
}


/**
 * The function checks if the given key exists in the hash map.
 * @param hash_map a hash map.
 * @param key the key to be checked.
 * @return 1 if the key is in the hash map, 0 otherwise.
 */

int HashMapContainsKey(HashMap *hash_map, KeyT key){
  return (HashMapAt(hash_map,key) != NULL);
}

/**
 * The function returns the value associated with the given key.
 * @param hash_map a hash map.
 * @param key the key to be checked.
 * @return the value associated with key if exists, NULL otherwise.
 */
ValueT HashMapAt(HashMap *hash_map, KeyT key){
  CONTAINS_CHECK_VALIDITY(hash_map, key)
  size_t bucket_idx, in_vector_idx;
  if (GetPairLocationByKey(key,hash_map,&bucket_idx,&in_vector_idx)) {
    return ((Pair*) hash_map->buckets[bucket_idx]->data[in_vector_idx])->value;
  }
  return NULL;
}

/**
 * The function erases the pair associated with key.
 * @param hash_map a hash map.
 * @param key a key of the pair to be erased.
 * @return 1 if the erasing was done successfully, 0 otherwise.
 */
int HashMapErase(HashMap *hash_map, KeyT key){
  CONTAINS_CHECK_VALIDITY(hash_map, key)
  size_t bucket_idx, in_vector_idx;
  if (GetPairLocationByKey(key,hash_map,&bucket_idx,&in_vector_idx)) {
    if (VectorErase(hash_map->buckets[bucket_idx], in_vector_idx)){
      hash_map->size--;
      ResizeMap(hash_map, REMOVE);
      return  SUCCESS;
    }
  }
  return NOT_FOUND;
}

/**
 * This function returns the load factor of the hash map.
 * @param hash_map a hash map.
 * @return the hash map's load factor, -1 if the function failed.
 */
double HashMapGetLoadFactor(HashMap *hash_map){
  if (hash_map&&(hash_map->capacity)){
    return (((double)hash_map->size)/hash_map->capacity);
  }
  return (double) FAIL;
}

/**
 * This function deletes all the elements in the hash map.
 * @param hash_map a hash map to be cleared.
 */
void HashMapClear(HashMap *hash_map){
  if(hash_map&&(hash_map->buckets)){
    size_t prev_capacity;

    while (hash_map->size > 0){
      prev_capacity = hash_map->capacity;
      for (size_t i = 0; i < (hash_map->capacity); ++i){
        if(prev_capacity != hash_map->capacity) break;
        Vector *vector = hash_map->buckets[i];
        size_t size = vector->size;
        for (size_t j = size; j > 0; --j) {
          if(prev_capacity != hash_map->capacity) break;
          Pair *elem = VectorAt(vector, j-1);
          HashMapErase(hash_map, elem->key);
        }
      }
    }

  }
}


