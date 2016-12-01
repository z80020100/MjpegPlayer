//
// Created by CLIFF on 2016/8/29.
//

#include <com_example_v002060_mjpegplayer_NativeOpenGLNV21Renderer.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <android/log.h>
#define LOG_TAG "opengl_nv21_renderer.c"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO , LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN , LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

const GLfloat TEXCOORDS[] = {
	0.0f, 0.0f,	// 左上
	0.0f, 1.0f,	// 左下
	1.0f, 0.0f,	// 右上
	1.0f, 1.0f	// 右下
};

const GLfloat VERTEXS[] = {
	-1.0f,  1.0f, 0.0f,	// 左上
	-1.0f, -1.0f, 0.0f,	// 左下
	 1.0f,  1.0f, 0.0f,	// 右上
	 1.0f, -1.0f, 0.0f	// 右下
};

const char VERTEX_SHADER[] =
    "attribute vec4 position;"
    "attribute vec4 inputTextureCoordinate;"
    "varying vec2 textureCoordinate;"
    "void main() {"
    	"gl_Position = position;"
    	"textureCoordinate = inputTextureCoordinate.xy;"
    "}";

const char FRAGMENT_SHADER[] =
    "precision mediump float;"
    "varying vec2 textureCoordinate;"
    "uniform sampler2D inputImageTexture;"
    "uniform sampler2D uvTexture;"
    "void main() {"
    	"vec4 y = vec4((texture2D(inputImageTexture, textureCoordinate).r - 16./255.) * 1.164);"
      	"vec4 u = vec4(texture2D(uvTexture, textureCoordinate).a - 128./255.);"
      	"vec4 v = vec4(texture2D(uvTexture, textureCoordinate).r - 128./255.);"
      	"y += v * vec4(1.596, -0.813, 0, 0);"
      	"y += u * vec4(0, -0.392, 2.017, 0);"
      	"y.a = 1.0;"
      	"gl_FragColor = y;"
    "}";

void printGLString(const char *name, GLenum s) {
	const char *v = (const char *) glGetString(s);
	LOGI("GL %s = %s\n", name, v);
}

void checkGlError(const char* op) {
    GLint error;
	for (error = glGetError(); error; error = glGetError()) {
		LOGE("after %s() glError (0x%x)\n", op, error);
	}
}

GLuint loadShader(GLenum shaderType, const char *shaderCode){
	GLuint shader = glCreateShader(shaderType);
	if (shader != 0) {
		glShaderSource(shader, 1, &shaderCode, NULL);
		glCompileShader(shader);
		GLint compiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (!compiled) {
			GLint infoLen = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
			if (infoLen) {
				char* buf = (char*) malloc(infoLen);
				if (buf) {
					glGetShaderInfoLog(shader, infoLen, NULL, buf);
					LOGE("Could not compile shader %d:\n%s\n",shaderType, buf);
					free(buf);
				}
			}
			glDeleteShader(shader);
		}
	}
	return shader;
}

GLuint createProgram(const char* vertexCode, const char* fragmentCode) {
	// バーテックスシェーダをコンパイルします。
	GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexCode);
	if (!vertexShader) {
		LOGE("vertex missed!");
		return 0;
	}

	// フラグメントシェーダをコンパイルします。
	GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, fragmentCode);
	if (!pixelShader) {
		LOGE("fragment missed!");
		return 0;
	}

	// プログラムを生成して、プログラムへバーテックスシェーダとフラグメントシェーダを関連付けます。
	GLuint program = glCreateProgram();
	if (program) {
		// プログラムへバーテックスシェーダを関連付けます。
		glAttachShader(program, vertexShader);
		checkGlError("glAttachShader");
		// プログラムへフラグメントシェーダを関連付けます。
		glAttachShader(program, pixelShader);
		checkGlError("glAttachShader");

	    glBindAttribLocation(program, 0, "position");
        glBindAttribLocation(program, 1,"inputTextureCoordinate");

		glLinkProgram(program);
		GLint linkStatus = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
		if (linkStatus != GL_TRUE) {
			GLint bufLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
			if (bufLength) {
				char* buf = (char*) malloc(bufLength);
				if (buf) {
					glGetProgramInfoLog(program, bufLength, NULL, buf);
					LOGE("Could not link program:\n%s\n", buf);
					free(buf);
				}
			}
			LOGE("program missed!");
			glDeleteProgram(program);
			program = 0;
		}
	}

	return program;
}

GLuint ProgramHandler;
GLuint PositionHandler;
GLuint TextureCoordinateHandler;
GLuint GLUniformTexture;
GLuint GLUniformUvTexture;
GLuint GLUniformWidthHandler;
GLuint GLUniformHeightHandler;

GLuint textures[2];
GLuint YTextureId;
GLuint UvTextureId;

extern char *y_data;
extern char *uv_data;
extern int renderer_lock;

int g_width, g_height;

unsigned int fps_count = 0;
struct timeval tv;
unsigned long fps_count_start_time_us;
unsigned long fps_count_cur_time_us;
int fps_show_interval = 3;

JNIEXPORT void JNICALL Java_com_example_v002060_mjpegplayer_NativeOpenGLNV21Renderer_nativeOnSurfaceCreated(JNIEnv *env, jobject renderer, jint width, jint height){
    LOGI("nativeOnSurfaceCreated");
}

JNIEXPORT void JNICALL Java_com_example_v002060_mjpegplayer_NativeOpenGLNV21Renderer_nativeOnSurfaceChanged(JNIEnv *env, jobject renderer, jint width, jint height){
    LOGI("nativeOnSurfaceChanged");

    g_width = width;
    g_height = height;

    	ProgramHandler = createProgram(VERTEX_SHADER, FRAGMENT_SHADER);
    	if (!ProgramHandler) {
    		LOGE("Could not create program.");
    	}

        PositionHandler = glGetAttribLocation(ProgramHandler, "position");
        TextureCoordinateHandler = glGetAttribLocation(ProgramHandler, "inputTextureCoordinate");
        GLUniformTexture = glGetUniformLocation(ProgramHandler, "inputImageTexture");
        GLUniformUvTexture = glGetUniformLocation(ProgramHandler, "uvTexture");
        GLUniformWidthHandler = glGetUniformLocation(ProgramHandler, "width");
        GLUniformHeightHandler = glGetUniformLocation(ProgramHandler, "height");

    	glUseProgram(ProgramHandler);
    	checkGlError("glUseProgram");

        glGenTextures(2, textures);
        YTextureId = textures[0];
        UvTextureId = textures[1];

        y_data = malloc(3840*2160);
        memset(y_data, 0, 3840*2160);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, YTextureId);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 3840, 2160, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, y_data);

        uv_data = malloc(3840*2160/2);
        memset(uv_data, 0, 3840*2160/2);
        glBindTexture(GL_TEXTURE_2D, UvTextureId);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, 3840 / 2, 2160 / 2, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, uv_data);
}

JNIEXPORT void JNICALL Java_com_example_v002060_mjpegplayer_NativeOpenGLNV21Renderer_nativeOnDrawFrame(JNIEnv *env, jobject renderer){

        //gettimeofday(&tv,NULL);
        //fps_count_start_time_us = 1000000 * tv.tv_sec + tv.tv_usec;

        glBindTexture(GL_TEXTURE_2D, YTextureId);
        //gettimeofday(&tv, NULL);
        //fps_count_cur_time_us = 1000000 * tv.tv_sec + tv.tv_usec;
        //LOGI("glBindTexture(GL_TEXTURE_2D, YTextureId) time: %f ms", (float)(fps_count_cur_time_us - fps_count_start_time_us)/1000);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 3840, 2160, 0,GL_LUMINANCE, GL_UNSIGNED_BYTE, y_data);
        //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 3840, 2160, GL_LUMINANCE, GL_UNSIGNED_BYTE, y_data);
        //gettimeofday(&tv, NULL);
        //fps_count_cur_time_us = 1000000 * tv.tv_sec + tv.tv_usec;
        //LOGI("glTexImage2D Y data time: %f ms", (float)(fps_count_cur_time_us - fps_count_start_time_us)/1000);

        glBindTexture(GL_TEXTURE_2D, UvTextureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, 3840 / 2, 2160 / 2, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, uv_data);
        //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 3840/2, 2160/2, GL_LUMINANCE, GL_UNSIGNED_BYTE, uv_data);
        //gettimeofday(&tv, NULL);
        //fps_count_cur_time_us = 1000000 * tv.tv_sec + tv.tv_usec;
        //LOGI("glTexImage2D UV data time: %f ms", (float)(fps_count_cur_time_us - fps_count_start_time_us)/1000);

        glUseProgram(ProgramHandler);
        //gettimeofday(&tv, NULL);
        //fps_count_cur_time_us = 1000000 * tv.tv_sec + tv.tv_usec;
        //LOGI("glUseProgram time: %f ms", (float)(fps_count_cur_time_us - fps_count_start_time_us)/1000);
        // Clears the screen and depth buffer.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //gettimeofday(&tv, NULL);
        //fps_count_cur_time_us = 1000000 * tv.tv_sec + tv.tv_usec;
        //LOGI("glClear time: %f ms", (float)(fps_count_cur_time_us - fps_count_start_time_us)/1000);

        // Draw our scene.
        glVertexAttribPointer(PositionHandler, 3, GL_FLOAT, 0, 0, VERTEXS);
        glEnableVertexAttribArray(PositionHandler);
        glVertexAttribPointer(TextureCoordinateHandler, 2, GL_FLOAT, 0, 0, TEXCOORDS);
        glEnableVertexAttribArray(TextureCoordinateHandler);
        glUniform1f(GLUniformWidthHandler, (float)g_width);
        glUniform1f(GLUniformHeightHandler, (float)g_height);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, YTextureId);
        glUniform1i(GLUniformTexture, 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, UvTextureId);
        glUniform1i(GLUniformUvTexture, 1);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDisableVertexAttribArray(PositionHandler);
        glDisableVertexAttribArray(TextureCoordinateHandler);
        glBindTexture(GL_TEXTURE_2D, 0);

        gettimeofday(&tv, NULL);
        fps_count_cur_time_us = 1000000 * tv.tv_sec + tv.tv_usec;
        //LOGI("nativeOnDrawFrame total time: %f ms", (float)(fps_count_cur_time_us - fps_count_start_time_us)/1000);

        // show FPS
        if(fps_count == 0){
            gettimeofday(&tv,NULL);
            fps_count_start_time_us = 1000000 * tv.tv_sec + tv.tv_usec;
        }

        gettimeofday(&tv, NULL);
        fps_count_cur_time_us = 1000000 * tv.tv_sec + tv.tv_usec;
        fps_count++;

        if((fps_count_cur_time_us - fps_count_start_time_us) >= fps_show_interval*1000000){

            LOGI("Frame Update FPS = %d", fps_count/fps_show_interval);
            fps_count = 0;
        }
}