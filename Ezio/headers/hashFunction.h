//
// Created by ezio on 28/08/19.
//


unsigned int hashFunction(struct image imageTest)
{
    int key = 0;

    //here comes the magic, the hash function
    key += imageTest.name[0]*11 + imageTest.name[1]*3;
    key += imageTest.height*imageTest.width;
    key *= imageTest.quality+1;
    key += (imageTest.name[0]+imageTest.name[1])*imageTest.name[2]*imageTest.isPng+1;

    return key%MAX_HASH_KEYS;
}
