//
// Created by ezio on 29/08/19.
//

int imageMagikMock(char *imagePath)
{
    //read dell'imagine da disco

    int im_fd;

    //printf("Path nella mock: %s\n", imagePath);

    //giovanni gay

    if((im_fd = open(imagePath+1, O_RDONLY)) == -1){
        perror("file not Found");
        setting.error = true;
        strcpy(setting.statusCode, NF);
        return -1;
    }

    int image_byte = lseek(im_fd, 0, SEEK_END);

    lseek(im_fd, 0, SEEK_SET);

    if((setting.payloadSize = read(im_fd, imageToInsert.imageBytes , image_byte)) == -1){
        perror("Error in reading the image");
        setting.error = true;
        strcpy(setting.statusCode, ISE );
        return -1;
    }

    imageToInsert.imageSize = setting.payloadSize;

    if(imageToInsert.isPng == 2)
        imageToInsert.isPng = rand()%1;

    return 0;
}
