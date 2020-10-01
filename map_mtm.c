//
//
#include <malloc.h>
#include <stdio.h>
#include <assert.h>
#include "map_mtm.h"

typedef struct element *Element;

Element elementCreate(MapDataElement data, MapKeyElement key,
                      copyMapDataElements copyDataElement,
                      copyMapKeyElements copyKeyElement);

void elementDestroy(Element element,freeMapDataElements freeDataElement,
                    freeMapKeyElements freeKeyElement);

void printLastKeysByOrder(Map map);

Element getLastElement(Map map);

MapDataElement changeDataOfIterator(Map map,MapDataElement dataElement);

MapResult setFirst(Map map,MapDataElement dataElement,MapKeyElement keyElement,
              Element first_element);

MapResult setBetween(Map map,MapDataElement dataElement,
                     MapKeyElement keyElement, Element last_element);

//this struct keeps the elementKey and elementData as fields in the struct
//all changes of keys and data will pass through this struct
struct element {
    MapDataElement element_data;
    MapKeyElement element_key;
    Element next_element;
};

struct Map_t{
    Element elements_list;
    copyMapDataElements copyDataElements;
    copyMapKeyElements copyKeyElements;
    freeMapDataElements freeDataElements;
    freeMapKeyElements freeKeyElements;
    compareMapKeyElements compareKeyElements;
    Element iterator;
    MapResult result;
};

/*
 *                              functions
 *
 *
 *                              Elements
 *
 *
 *
 */

Element elementCreate(MapDataElement data, MapKeyElement key,
                      copyMapDataElements copyDataElement,
                      copyMapKeyElements copyKeyElement){
    if ((!data)||(!key)){
        printf("Element null argument");
        return NULL;
    }
    Element new_element = malloc(sizeof(*new_element));
    new_element->element_data = copyDataElement(data);
    new_element->element_key = copyKeyElement(key);
    new_element->next_element = NULL;
    return new_element;
}

void elementDestroy(Element element,freeMapDataElements freeDataElement,
                    freeMapKeyElements freeKeyElement){
    freeKeyElement(element->element_key);
    freeDataElement(element->element_data);
    free(element);
}

Element elementCopy(Element element, copyMapDataElements copyDataElement,
                    copyMapKeyElements copyKeyElement){
    if (!element) return NULL;
    return elementCreate(element->element_data, element->element_key,
                         copyDataElement, copyKeyElement);
}


/*
 *
 *
 *                      Map
 */

Map mapCreate(copyMapDataElements copyDataElement, copyMapKeyElements copyKeyElement,
              freeMapDataElements freeDataElement, freeMapKeyElements freeKeyElement,
              compareMapKeyElements compareKeyElements){
    if (!copyDataElement||!copyKeyElement||!freeDataElement||!freeKeyElement||
            !compareKeyElements) return NULL;
    Map new_map = malloc(sizeof(*new_map));
    if (new_map==NULL){
        printf("Allocation error\n");
        return NULL;
    }
    new_map->copyDataElements =copyDataElement;
    new_map->copyKeyElements =copyKeyElement;
    new_map->freeDataElements =freeDataElement;
    new_map->freeKeyElements =freeKeyElement;
    new_map->compareKeyElements =compareKeyElements;
    new_map->elements_list=NULL;
    //iterator points at an Element type (as a struct that we defined for
    // auxiliary) that has the data and key in it
    new_map->iterator=NULL;
    new_map->result=MAP_SUCCESS;
    return new_map;
}

void mapDestroy(Map map){
    if (map==NULL) return;
    while (map->elements_list){
        Element temp= map->elements_list;
        map->elements_list = map->elements_list->next_element;
        elementDestroy(temp,map->freeDataElements,map->freeKeyElements);
    }
    free(map);
}

Map mapCopy(Map map){
    if (!map) return NULL;
    Map new_map = mapCreate(map->copyDataElements,map->copyKeyElements,
                            map->freeDataElements,map->freeKeyElements,
                            map->compareKeyElements);
    if (new_map==NULL) return NULL;
    new_map->elements_list = elementCopy(map->elements_list,
                                         map->copyDataElements,
                                         map->copyKeyElements);
    map->iterator = map->elements_list;
    new_map->iterator = new_map->elements_list;
    while (map->iterator) {
        assert(map->elements_list != NULL);
        new_map->iterator->next_element =
                elementCopy(map->iterator->next_element,
                            map->copyDataElements, map->copyKeyElements);
        map->iterator = map->iterator->next_element;
        new_map->iterator = new_map->iterator->next_element;
    }
    return new_map;
}

int mapGetSize(Map map){
    if (map==NULL) return -1;
    int counter = 0;
    for(map->iterator = map->elements_list; map->iterator ;
        map->iterator = map->iterator->next_element){
        counter++;
    }
    return counter;
}

bool mapContains(Map map, MapKeyElement element) {
    if ((map==NULL)||(element==NULL)) return false;
    for (map->iterator = map->elements_list; map->iterator;
         map->iterator = map->iterator->next_element) {
        int compare =
                map->compareKeyElements(element,map->iterator->element_key);
        if (compare==0){
            return true;
        } else if (compare<0){
            return false;
        }
    }
    return false;
}

MapResult mapPut(Map map, MapKeyElement keyElement, MapDataElement dataElement){
    if (map==NULL) return MAP_NULL_ARGUMENT;
    if (!keyElement||!dataElement){
        map->iterator = NULL;
        return MAP_NULL_ARGUMENT;
    }
    Element last_element =  map->elements_list;
    for (map->iterator = map->elements_list;map->iterator;
         map->iterator = map->iterator->next_element) {
        int compare =
                map->compareKeyElements(keyElement,map->iterator->element_key);
        if (compare==0){ // key already exists
            if (!changeDataOfIterator(map,dataElement)){
                map->iterator=NULL;
                return MAP_OUT_OF_MEMORY;
            }
            map->iterator = NULL;
            return MAP_SUCCESS;
        }else if(compare<0){ // just past the place that key is to be put
            if (map->iterator==map->elements_list){
                //in case there are no other elements
                return setFirst(map,dataElement,keyElement,map->elements_list);
            }
            return setBetween(map,dataElement,keyElement,last_element);
            //between last element ant iterator
        }
        last_element= map->iterator;
    }
    if (!map->elements_list){
        return setFirst(map,dataElement,keyElement,NULL);
    }
    return  setBetween(map,dataElement,keyElement,getLastElement(map));
}

MapDataElement mapGet(Map map, MapKeyElement keyElement){
    if (map==NULL) return NULL;
    Element original_iterator = map->iterator;
    map->iterator = map->elements_list;
    for (map->iterator = map->elements_list; map->iterator;
         map->iterator = map->iterator->next_element) {
        int compare =
                map->compareKeyElements(keyElement,map->iterator->element_key);
        if (compare==0){//key found
            MapDataElement data = map->iterator->element_data;
            map->iterator = original_iterator;
            return data;
        }else if(compare<0) {//key not found
            map->iterator = original_iterator;
            return NULL;
        }
    }
    map->iterator = original_iterator;
    return NULL;
}

MapResult mapRemove(Map map, MapKeyElement keyElement) {
    if (!map||!keyElement) return MAP_NULL_ARGUMENT;
    map->iterator = map->elements_list;
    Element temp = map->elements_list;
    if (!(map->iterator))return MAP_ITEM_DOES_NOT_EXIST;
    //in case that the first Element is the one that needs to be removed:
    int compare= map->compareKeyElements(keyElement,map->iterator->element_key);
    if (compare==0){
        map->elements_list=map->elements_list->next_element;
        elementDestroy(map->iterator,map->freeDataElements,
                       map->freeKeyElements);
        return MAP_SUCCESS;
    }
    //else
    for (; map->iterator; map->iterator = map->iterator->next_element) {
        compare=map->compareKeyElements(keyElement,map->iterator->element_key);
        if (compare == 0) {
            temp->next_element = map->iterator->next_element;
            elementDestroy(map->iterator, map->freeDataElements,
                           map->freeKeyElements);
            return MAP_SUCCESS;
        } else if (compare < 0) return MAP_ITEM_DOES_NOT_EXIST;
        temp = map->iterator;
    }
    return MAP_ITEM_DOES_NOT_EXIST;
}

MapKeyElement mapGetFirst(Map map){
    if (!map) return NULL;
    map->iterator = map->elements_list;
    if (!map->elements_list) return NULL;
    return map->elements_list->element_key;
}

MapKeyElement mapGetNext(Map map) {
    if (!map||!map->iterator){
        return NULL;
    }
    map->iterator=map->iterator->next_element;
    if(!map->iterator){
        return NULL;
    }
    return map->iterator->element_key;
}

MapResult mapClear(Map map){
    if (!map) return MAP_NULL_ARGUMENT;
    Element temp;
    for (map->iterator=map->elements_list; map->iterator;) {
        temp= map->iterator;
        map->iterator=map->iterator->next_element;
        elementDestroy(temp,map->freeDataElements,map->freeKeyElements);
    }
    map->elements_list=NULL;
    return MAP_SUCCESS;
}

Element getLastElement(Map map) {
    if (!map) return NULL;
    Element save_iterator1= map->iterator ,save_iterator2;
    for (map->iterator=map->elements_list; map->iterator;
         map->iterator = map->iterator->next_element) {
        if (!map->iterator->next_element){
            save_iterator2=map->iterator;
            map->iterator=save_iterator1;
            return save_iterator2;
        }
    }
    map->iterator=save_iterator1;
    return NULL;
}

//This function receives a map and a mapDataElement type and returns a
//pointer to a new copied MapDataElement and changes the data of the
//map's iterator's element
MapDataElement changeDataOfIterator(Map map,MapDataElement dataElement){
    map->freeDataElements(map->iterator->element_data);
    map->iterator->element_data = map->copyDataElements(dataElement);
    return map->iterator->element_data;
}

//This function receives a map, a dataElement, a keyElement, and the first
// element in map, it puts in the first place of the map a new element with the
// data and key the function received ,and returns if it succeeded allocating
MapResult setFirst(Map map,MapDataElement dataElement,MapKeyElement keyElement,
              Element first_element){
    map->elements_list= elementCreate(dataElement,keyElement,
                                      map->copyDataElements,
                                      map->copyKeyElements);
    if (!map->elements_list) {
        map->iterator=NULL;
        return MAP_OUT_OF_MEMORY;
    }
    map->elements_list->next_element = first_element;
    map->iterator = NULL;
    return MAP_SUCCESS;
}

//This function receives a map, a dataElement, a keyElement, and the last
// element before the iterator, it puts in the place between the last Element
// before the iterator and the iterator, a new element with the data and key the
// function received ,and returns if it succeeded allocating
MapResult setBetween(Map map,MapDataElement dataElement,
                     MapKeyElement keyElement, Element last_element) {
    Element new_element = elementCreate(dataElement, keyElement,
                                        map->copyDataElements,
                                        map->copyKeyElements);
    if (!new_element) {
        map->iterator=NULL;
        return MAP_OUT_OF_MEMORY;
    }
    new_element->next_element=map->iterator;
    last_element->next_element=new_element;
    map->iterator = NULL;
    return MAP_SUCCESS;
}