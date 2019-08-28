//
// Created by ezio on 28/08/19.
//


unsigned int hashFunction(struct image imageTest)
{
    int key = 0;

    //here comes the magic, the hash function
    key += strlen(imageTest.name)*11;
    key += imageTest.height*imageTest.width;
    key *= imageTest.quality;
    key += imageTest.name[0];
    key += imageTest.name[1];
    key += rand();
    //key *= rand();

    return key%MAX_HASH_KEYS;
}
