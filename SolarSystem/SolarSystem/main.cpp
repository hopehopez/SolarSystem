
#include "GLTools.h"
#include "GLShaderManager.h"
#include "GLFrustum.h"
#include "GLBatch.h"
#include "GLMatrixStack.h"
#include "GLGeometryTransform.h"
#include "StopWatch.h"

#include <math.h>
#include <stdio.h>

#ifdef __APPLE__
#include <glut/glut.h>
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>
#endif

///着色器管理器
GLShaderManager shaderManager;
///模型视图矩阵
GLMatrixStack modelViewMatrix;
///投影矩阵
GLMatrixStack projectionMataix;
///视景体
GLFrustum viewFrunstum;
///几何图形变换通道
GLGeometryTransform transformPipeline;

///大球
GLTriangleBatch torusBatch;
///小球
GLTriangleBatch sphereBatch;
///地板
GLBatch floorBatch;


//角色帧 照相机角色帧
GLFrame   cameraFrame;
GLFrame  objectFrame;

#define NUM_SPHERES 50
GLFrame sepheres[NUM_SPHERES];

void SetupRC(){
    //1 初始化
    glClearColor(0, 0, 0, 1);
    shaderManager.InitializeStockShaders();
    
    //2 开启深度测试
    glEnable(GL_DEPTH_TEST);
    
    //3 设置地板顶点数据
    floorBatch.Begin(GL_LINES, 324);
    for(GLfloat x = -20.0; x <= 20.0f; x+= 0.5) {
        floorBatch.Vertex3f(x, -0.55f, 20.0f);
        floorBatch.Vertex3f(x, -0.55f, -20.0f);
        
        floorBatch.Vertex3f(20.0f, -0.55f, x);
        floorBatch.Vertex3f(-20.0f, -0.55f, x);
    }
    floorBatch.End();
    
    
    //设置大球
    gltMakeSphere(torusBatch, 0.4f, 40, 80);
    
    //5. 设置小球球模型
    gltMakeSphere(sphereBatch, 0.1f, 13, 26);
    for(int i=0; i<NUM_SPHERES; i++) {
        GLfloat x = ((GLfloat)((rand() % 400) - 200 ) * 0.1f);
        GLfloat z = ((GLfloat)((rand() % 400) - 200 ) * 0.1f);
        sepheres[i].SetOrigin(x, 0.0f, z);
    }
}



//进行调用以绘制场景
void RenderScene(void){
    //清除缓冲器
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    //颜色值
    static GLfloat vFloorColor[] = {0.0, 1.0, 0.0, 1.0};
    //大球颜色
    static GLfloat vTorusColor[] = {1.0, 0, 0, 1};
    //小球颜色
    static GLfloat vSphereColor[] = {0, 0, 1, 1};
    
    //定时器
    static CStopWatch rotTimer;
    float yRot = rotTimer.GetElapsedSeconds()*60.f;
    
    //空栈
    //单元矩阵 -> 旋转 -> 移动 -> 缩放 -> 出栈
    modelViewMatrix.PushMatrix();
    
    M3DMatrix44f mCamera;
    cameraFrame.GetCameraMatrix(mCamera);
    modelViewMatrix.PushMatrix(mCamera);
    
    //绘制地面
    shaderManager.UseStockShader(GLT_SHADER_FLAT,
                                 transformPipeline.GetModelViewProjectionMatrix(),
                                 vFloorColor);
    floorBatch.Draw();
    
    //红色的球
    //设置光源位置
    M3DMatrix44f vlightPos = {0, 10, 5, 1};
    
    //6.使得大球位置平移(3.0)向屏幕里面
    modelViewMatrix.Translate(0, 0, -3);
    //7.压栈(复制栈顶) 只移动一次 这样不会和旋转混合效果到一起
    modelViewMatrix.PushMatrix();
    
    //8.大球自转
    modelViewMatrix.Rotate(yRot, 0, 1, 0);
    shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF,
                                 transformPipeline.GetModelViewMatrix(),
                                 transformPipeline.GetProjectionMatrix(),
                                 vlightPos,
                                 vTorusColor);
    torusBatch.Draw();
    modelViewMatrix.PopMatrix();
    
    //画50个小球
    for (int i=0; i<NUM_SPHERES; i++) {
        modelViewMatrix.PushMatrix();
        modelViewMatrix.MultMatrix(sepheres[i]);
        shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF,
                                     transformPipeline.GetModelViewMatrix(),
                                     transformPipeline.GetProjectionMatrix(),
                                     vlightPos,
                                     vSphereColor);
        sphereBatch.Draw();
        modelViewMatrix.PopMatrix();
    }
    
    modelViewMatrix.Rotate(yRot*2, 0, 1, 0);
    modelViewMatrix.Translate(0.8, 0, 0);
    shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF,
                                 transformPipeline.GetModelViewMatrix(),
                                 transformPipeline.GetProjectionMatrix(),
                                 vlightPos,
                                 vSphereColor);
    sphereBatch.Draw();
    
    modelViewMatrix.PopMatrix();
    modelViewMatrix.PopMatrix();
    //交换缓冲区
    glutSwapBuffers();
    
    glutPostRedisplay();
}

//屏幕更改大小或已初始化
void ChangeSize(int nWidth, int nHeight){
    //设置视口
    glViewport(0, 0, nWidth, nHeight);
    
    //创建透视投影
    viewFrunstum.SetPerspective(35, float(nWidth)/float(nHeight), 1.0f, 100.0f);
    //加载到投影矩阵堆栈
    projectionMataix.LoadMatrix(viewFrunstum.GetProjectionMatrix());
    
    //3.设置变换管道以使用两个矩阵堆栈（变换矩阵modelViewMatrix ，投影矩阵projectionMatrix）
    //初始化GLGeometryTransform 的实例transformPipeline.通过将它的内部指针设置为模型视图矩阵堆栈 和 投影矩阵堆栈实例，来完成初始化
    //当然这个操作也可以在SetupRC 函数中完成，但是在窗口大小改变时或者窗口创建时设置它们并没有坏处。而且这样可以一次性完成矩阵和管线的设置。
    transformPipeline.SetMatrixStacks(modelViewMatrix, projectionMataix);
}

void SpeacialKeys(int key,int x,int y){
    float liner = 0.1f;
    float angular = float(m3dDegToRad(5.0f));
    
    if (key == GLUT_KEY_UP) {
        cameraFrame.MoveForward(liner);
    }
    if (key == GLUT_KEY_DOWN) {
        cameraFrame.MoveForward(-liner);
    }
    if (key == GLUT_KEY_LEFT) {
        cameraFrame.RotateWorld(angular, 0, 1, 0);
    }
    if (key == GLUT_KEY_RIGHT) {
        cameraFrame.RotateWorld(-angular, 0, 1, 0);
    }
}

int main(int argc, char* argv[]){
    gltSetWorkingDirectory(argv[0]);
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("OpenGL SphereWorld");
    
    glutReshapeFunc(ChangeSize);
    glutDisplayFunc(RenderScene);
    glutSpecialFunc(SpeacialKeys);
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
        return 1;
    }
    
    SetupRC();
    glutMainLoop();
    return 0;
    
}
