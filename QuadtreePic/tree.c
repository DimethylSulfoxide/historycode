#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <inttypes.h>
#include <float.h>
#include <unistd.h>
#include <pthread.h>
#include <unistd.h>

typedef struct pixel
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
} pixel;

typedef struct picture
{
    int width;
    int height;
    pixel ** image_p2;
} picture;

typedef struct quadnode
{
    // position
    int left, right, upper, lower;

    // sons
    struct quadnode *leftupper, *leftlower, *rightupper, *rightlower;
} quadnode;

typedef struct ad {
    double ave_r;
    double ave_g;
    double ave_b;
    double variance_r;
    double variance_g;
    double variance_b;
    double variance_total;
} ave_data;


typedef struct mtarg{
    quadnode * node;
    double tolerance;
    pixel ** data_p2;
    ave_data * ave_data_p;
} mul_thrd_args;

// ave_data * get_block_variance_rgbave(int, int, int, int, pixel**, ave_data*);
ave_data * get_block_variance_rgbave(quadnode*, pixel**, ave_data*);
quadnode * create_quadtree(quadnode * root, double tolerance, pixel**, ave_data*);
picture * read_image(char * file_name);
void adjust_pixel(quadnode * node, ave_data * ave_p, pixel** img_p2);
int save_image(char * filename, picture * imp_p);


// double minvar = LDBL_MAX, maxvar = 0.;

// typedef quadnode * (*create_quadtree_single_thread)(quadnode * root, double tolerance, pixel**, ave_data*);
typedef void*(*voidfptr)(void*);
void * create_quadtree_sgle_thread(mul_thrd_args*);
quadnode * create_quadtree_multi_threads(quadnode * root, double tolerance, pixel**, ave_data*, void*(*)(void*));

int main(int argc, char * argv[]) {
    // time_t starttime = time(NULL);
    srand(time(NULL));
    char infilename[100] = {0};
    char outfilename[100] = {0};
    char tolerancetmp[100] = {0};
    double tolerance = 0.;
    int multi_threads_on = 0, ret;
    char** end_tmp;

    while ((ret = getopt(argc, argv, "i:o:t:m")) != -1) {
        switch (ret) {
            case 'm':
                multi_threads_on = 1;
                break;
            case 'i':
                memcpy(infilename, optarg, sizeof(char[100]));
                break;
            case 'o':
                memcpy(outfilename, optarg, sizeof(char[100]));
                break;
            case 't':
                // memcpy(tolerancetmp, optarg, sizeof(char[100]));
                tolerance = atof(optarg);
                break;
        }
    }

    // printf("%s\n%s\n%lf\t%d\n", infilename, outfilename, tolerance, multi_threads_on);

    picture * p = read_image(infilename);
    quadnode * root = (quadnode*)malloc(sizeof(quadnode));
    ave_data * ave_data_p = (ave_data*)malloc(sizeof(ave_data));
    root->left = 1;
    root->upper = 1;
    root->right = p->width;
    root->lower = p->height;


    // to be finished

    // create_quadtree_single_thread create_quadtree_sgl = create_quadtree;
    if (multi_threads_on)
        create_quadtree_multi_threads(root, tolerance, p->image_p2, ave_data_p, create_quadtree_sgle_thread);
    else
        root = create_quadtree(root, tolerance, p->image_p2, ave_data_p);

    save_image(outfilename, p);
    // time_t endtime = time(NULL);
    // printf("Takes %lf seconds.", difftime(endtime, starttime));
    return 0;
}

picture * read_image(char * file_name) {
    FILE * fp = fopen(file_name, "rb");
    picture * image_p = (picture*)malloc(sizeof(picture));
    fscanf(fp, "%*s%d%d%*d%*c", &image_p->width, &image_p->height);
    image_p->image_p2 = (pixel**)malloc(sizeof(pixel*) * image_p->height);
    for (int i = 0; i < image_p->height; i ++) {
        image_p->image_p2[i] = (pixel*)malloc(sizeof(pixel) * image_p->width);
        fread(image_p->image_p2[i], sizeof(pixel), image_p->width, fp);
    }
    fclose(fp);
    return image_p;
}

int save_image(char * filename, picture * img_p) {
    FILE * fp = fopen(filename, "wb");
    fprintf(fp, "P6\n%d %d\n255\n", img_p->width, img_p->height);

    for (int i = 0; i < img_p->height; i ++) {
        fwrite(img_p->image_p2[i], sizeof(pixel), img_p->width, fp);
    }

    fclose(fp);
    return 0;
}

ave_data * get_block_variance_rgbave(quadnode * node, pixel** data_p2, ave_data* ave_data_p) {
    // 四个坐标均为闭区间
    double ave_r = 0., ave_g = 0., ave_b = 0., v_r = 0., v_g = 0., v_b = 0.;
    double total_num = (node->right - node->left + 1) * (node->lower - node->upper + 1);
    for (int i = node->upper - 1; i <= node->lower - 1; i ++) {
        for (int j = node->left - 1; j <= node->right - 1; j ++) {
            ave_r += 1.0 * data_p2[i][j].r / total_num;
            ave_g += 1.0 * data_p2[i][j].g / total_num;
            ave_b += 1.0 * data_p2[i][j].b / total_num;
        }
    }
    ave_data_p->ave_r = ave_r;
    ave_data_p->ave_g = ave_g;
    ave_data_p->ave_b = ave_b;

    for (int i = node->upper - 1; i <= node->lower - 1; i ++) {
        for (int j = node->left - 1; j <= node->right - 1; j ++) {
            v_r += (data_p2[i][j].r - ave_r) * (data_p2[i][j].r - ave_r) / total_num;
            v_g += (data_p2[i][j].g - ave_g) * (data_p2[i][j].g - ave_g) / total_num;
            v_b += (data_p2[i][j].b - ave_b) * (data_p2[i][j].b - ave_b) / total_num;
            // v_r += powl(data_p2[i][j].r - ave_r, 2) / total_num;
            // v_g += powl(data_p2[i][j].g - ave_g, 2) / total_num;
            // v_b += powl(data_p2[i][j].b - ave_b, 2) / total_num;
        }
    }
    ave_data_p->variance_r = v_r;
    ave_data_p->variance_g = v_g;
    ave_data_p->variance_b = v_b;

    ave_data_p->variance_total = (v_r + v_g + v_b) / 3.0;

    // printf(">>> dbg: %lf <<<\n", ave_data_p->variance_total);
    // printf("%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t\n", 
    //       ave_data_p->ave_r, ave_data_p->ave_g, ave_data_p->ave_b,
    //       ave_data_p->variance_r, ave_data_p->variance_g, ave_data_p->variance_b,
    //       ave_data_p->variance_total, total_num);
    // if (ave_data_p->variance_total > maxvar)
    //     maxvar = ave_data_p->variance_total;
    // if (ave_data_p->variance_total < minvar)
    //     minvar = ave_data_p->variance_total;

    return ave_data_p;
}

quadnode * create_quadtree(quadnode * root, double tolerance, pixel ** data_p2, ave_data * ave_p) {
    // printf("dbg >> root is %x, data_p2 is %x. << dbg \n", root, data_p2);
    // root为手动四分之后的其中一部分,一定不为空
    int t_left = root->left, 
        t_right = root->right,
        t_upper = root->upper,
        t_lower = root->lower, 
        t_mid_vertical = 0, 
        t_mid_horizontal = 0;

    // quadnode * curr = (quadnode*)malloc(sizeof(quadnode));
    // ave_data * ave_p;
    ave_p = get_block_variance_rgbave(root, data_p2, ave_p);

    // if (t_left == t_right || t_upper == t_lower) {
    if (t_left == t_right || t_upper == t_lower || ave_p->variance_total < tolerance) {
        root->leftlower = NULL;
        root->leftupper = NULL;
        root->rightlower = NULL;
        root->rightupper = NULL;
        // return NULL;

        adjust_pixel(root, ave_p, data_p2);
        // 该块需要进行模糊
        // 将该块内所有rgb值设为平均值
        return root;
    }

    else {

        t_mid_horizontal = (t_upper + t_lower - 1) / 2;
        t_mid_vertical = (t_left + t_right - 1) / 2;
        // curr->
        root->leftlower = (quadnode*)malloc(sizeof(quadnode));
        root->leftupper = (quadnode*)malloc(sizeof(quadnode));
        root->rightlower = (quadnode*)malloc(sizeof(quadnode));
        root->rightupper = (quadnode*)malloc(sizeof(quadnode));

        root->leftupper->left = t_left;
        root->leftupper->right = t_mid_vertical;
        root->leftupper->upper = t_upper;
        root->leftupper->lower = t_mid_horizontal;

        root->leftlower->left = t_left;
        root->leftlower->right = t_mid_vertical;
        root->leftlower->upper = t_mid_horizontal + 1;
        root->leftlower->lower = t_lower;

        root->rightupper->left = t_mid_vertical + 1;
        root->rightupper->right = t_right;
        root->rightupper->upper = t_upper;
        root->rightupper->lower = t_mid_horizontal;

        root->rightlower->left = t_mid_vertical + 1;
        root->rightlower->right = t_right;
        root->rightlower->upper = t_mid_horizontal + 1;
        root->rightlower->lower = t_lower;

        root->leftupper = create_quadtree(root->leftupper, tolerance, data_p2, ave_p);
        root->leftlower = create_quadtree(root->leftlower, tolerance, data_p2, ave_p);
        root->rightupper = create_quadtree(root->rightupper, tolerance, data_p2, ave_p);
        root->rightlower = create_quadtree(root->rightlower, tolerance, data_p2, ave_p);

        return root;
    }

}

void adjust_pixel(quadnode * node, ave_data * ave_p, pixel** img_p2) {
    pixel pixel_tmp = {
        (unsigned char) (ave_p->ave_r),
        (unsigned char) (ave_p->ave_g),
        (unsigned char) (ave_p->ave_b)
    };
    
    for (int i = node->upper - 1; i <= node->lower - 1; i ++) {
        for (int j = node->left - 1; j <= node->right - 1; j ++) {

            memcpy(&img_p2[i][j], &pixel_tmp, sizeof(pixel));

        }
    } 
}

quadnode * create_quadtree_multi_threads(quadnode * root, double tolerance, pixel** data_p2, ave_data* ave_p, voidfptr cq_single_thread) {
    int t_left = root->left, 
        t_right = root->right,
        t_upper = root->upper,
        t_lower = root->lower, 
        t_mid_vertical = 0, 
        t_mid_horizontal = 0;

    // quadnode * curr = (quadnode*)malloc(sizeof(quadnode));
    // ave_data * ave_p;
    ave_p = get_block_variance_rgbave(root, data_p2, ave_p);

    // if (t_left == t_right || t_upper == t_lower) {
    if (t_left == t_right || t_upper == t_lower || ave_p->variance_total < tolerance) {
        root->leftlower = NULL;
        root->leftupper = NULL;
        root->rightlower = NULL;
        root->rightupper = NULL;
        // return NULL;

        adjust_pixel(root, ave_p, data_p2);
        // 该块需要进行模糊
        // 将该块内所有rgb值设为平均值
        return root;
    }

    else {

        t_mid_horizontal = (t_upper + t_lower - 1) / 2;
        t_mid_vertical = (t_left + t_right - 1) / 2;
        // curr->
        root->leftlower = (quadnode*)malloc(sizeof(quadnode));
        root->leftupper = (quadnode*)malloc(sizeof(quadnode));
        root->rightlower = (quadnode*)malloc(sizeof(quadnode));
        root->rightupper = (quadnode*)malloc(sizeof(quadnode));

        root->leftupper->left = t_left;
        root->leftupper->right = t_mid_vertical;
        root->leftupper->upper = t_upper;
        root->leftupper->lower = t_mid_horizontal;

        root->leftlower->left = t_left;
        root->leftlower->right = t_mid_vertical;
        root->leftlower->upper = t_mid_horizontal + 1;
        root->leftlower->lower = t_lower;

        root->rightupper->left = t_mid_vertical + 1;
        root->rightupper->right = t_right;
        root->rightupper->upper = t_upper;
        root->rightupper->lower = t_mid_horizontal;

        root->rightlower->left = t_mid_vertical + 1;
        root->rightlower->right = t_right;
        root->rightlower->upper = t_mid_horizontal + 1;
        root->rightlower->lower = t_lower;


        // 开始分线程

        ave_data * ave_data_p = (ave_data*)malloc(sizeof(ave_data) * 4);
        // ave_data** ave_data_p2 = &ave_data_p;
        mul_thrd_args *args = (mul_thrd_args*)malloc(sizeof(mul_thrd_args) * 4);

        pthread_t threads[4];
        quadnode ** thrd_ret[4];
        thrd_ret[0] = &(root->leftupper);
        thrd_ret[1] = &(root->leftlower);
        thrd_ret[2] = &(root->rightupper);
        thrd_ret[3] = &(root->rightlower);
        args[0].node = root->leftupper;
        args[1].node = root->leftlower;
        args[2].node = root->rightupper;
        args[3].node = root->rightlower;

        args[0].ave_data_p = ave_data_p;
        args[1].ave_data_p = ave_data_p + 1;
        args[2].ave_data_p = ave_data_p + 2;
        args[3].ave_data_p = ave_data_p + 3;
        // args[0].ave_data_p = ave_data_p2[0];
        // args[1].ave_data_p = ave_data_p2[1];
        // args[2].ave_data_p = ave_data_p2[2];
        // args[3].ave_data_p = ave_data_p2[3];

        for (int i = 0; i < 4; i ++) {
            args[i].data_p2 = data_p2;
            args[i].tolerance = tolerance;
        }

        for (int i = 0; i < 4; i ++)
            pthread_create(&threads[i], NULL, (void *(*)(void *))cq_single_thread, &args[i]);

        for (int i = 0; i < 4; i ++)
            pthread_join(threads[i], thrd_ret[i]);

        // root->leftupper = create_quadtree(root->leftupper, tolerance, data_p2, ave_p);
        // root->leftlower = create_quadtree(root->leftlower, tolerance, data_p2, ave_p);
        // root->rightupper = create_quadtree(root->rightupper, tolerance, data_p2, ave_p);
        // root->rightlower = create_quadtree(root->rightlower, tolerance, data_p2, ave_p);

        return root;
    }
}

void * create_quadtree_sgle_thread(mul_thrd_args* args) {
    // args = (mul_thrd_args*)args;
    quadnode * root = args->node;
    double tolerance = args->tolerance;
    pixel ** data_p2 = args->data_p2;
    ave_data * ave_p = args->ave_data_p;


    int t_left = root->left, 
        t_right = root->right,
        t_upper = root->upper,
        t_lower = root->lower, 
        t_mid_vertical = 0, 
        t_mid_horizontal = 0;

    // quadnode * curr = (quadnode*)malloc(sizeof(quadnode));
    // ave_data * ave_p;
    ave_p = get_block_variance_rgbave(root, data_p2, ave_p);

    // if (t_left == t_right || t_upper == t_lower) {
    if (t_left == t_right || t_upper == t_lower || ave_p->variance_total < tolerance) {
        root->leftlower = NULL;
        root->leftupper = NULL;
        root->rightlower = NULL;
        root->rightupper = NULL;
        // return NULL;

        adjust_pixel(root, ave_p, data_p2);
        // 该块需要进行模糊
        // 将该块内所有rgb值设为平均值
        // return root;
        pthread_exit(root);
    }

    else {

        t_mid_horizontal = (t_upper + t_lower - 1) / 2;
        t_mid_vertical = (t_left + t_right - 1) / 2;
        // curr->
        root->leftlower = (quadnode*)malloc(sizeof(quadnode));
        root->leftupper = (quadnode*)malloc(sizeof(quadnode));
        root->rightlower = (quadnode*)malloc(sizeof(quadnode));
        root->rightupper = (quadnode*)malloc(sizeof(quadnode));

        root->leftupper->left = t_left;
        root->leftupper->right = t_mid_vertical;
        root->leftupper->upper = t_upper;
        root->leftupper->lower = t_mid_horizontal;

        root->leftlower->left = t_left;
        root->leftlower->right = t_mid_vertical;
        root->leftlower->upper = t_mid_horizontal + 1;
        root->leftlower->lower = t_lower;

        root->rightupper->left = t_mid_vertical + 1;
        root->rightupper->right = t_right;
        root->rightupper->upper = t_upper;
        root->rightupper->lower = t_mid_horizontal;

        root->rightlower->left = t_mid_vertical + 1;
        root->rightlower->right = t_right;
        root->rightlower->upper = t_mid_horizontal + 1;
        root->rightlower->lower = t_lower;

        root->leftupper = create_quadtree(root->leftupper, tolerance, data_p2, ave_p);
        root->leftlower = create_quadtree(root->leftlower, tolerance, data_p2, ave_p);
        root->rightupper = create_quadtree(root->rightupper, tolerance, data_p2, ave_p);
        root->rightlower = create_quadtree(root->rightlower, tolerance, data_p2, ave_p);

        // return root;
        pthread_exit(root);
    }

}