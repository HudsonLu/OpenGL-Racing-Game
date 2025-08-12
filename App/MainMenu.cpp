#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>

#define STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font.h"

static inline float clampf(float v,float lo,float hi){return v<lo?lo:(v>hi?hi:v);}

static const char* options[]={"Original Track","Circular Track","Night Track","Race Track"};
static int  selection=0;
static bool confirmed=false;

static void drawRect(float x,float y,float w,float h,float r,float g,float b,float a){
    glColor4f(r,g,b,a); glBegin(GL_QUADS);
    glVertex2f(x,y); glVertex2f(x+w,y); glVertex2f(x+w,y+h); glVertex2f(x,y+h);
    glEnd();
}
static void drawOutlineRect(float x,float y,float w,float h,float r,float g,float b,float a,float lw=2.f){
    glLineWidth(lw); glColor4f(r,g,b,a); glBegin(GL_LINE_LOOP);
    glVertex2f(x,y); glVertex2f(x+w,y); glVertex2f(x+w,y+h); glVertex2f(x,y+h);
    glEnd();
}
static void textBounds(const char* s,float& outW,float& outH){
    static char buf[64*1024];
    int q=stb_easy_font_print(0,0,(char*)s,nullptr,buf,sizeof(buf));
    const float* v=(const float*)buf; int verts=q*4;
    float minx=1e9f,maxx=-1e9f,miny=1e9f,maxy=-1e9f;
    for(int i=0;i<verts;i++){ float x=v[i*4+0], y=v[i*4+1];
        if(x<minx)minx=x; if(x>maxx)maxx=x; if(y<miny)miny=y; if(y>maxy)maxy=y; }
    outW=verts?maxx-minx:0.f; outH=verts?maxy-miny:0.f;
}
static void drawText(float x,float y,float s,const char* t,float r,float g,float b,float a,bool shadow=true){
    static char buf[64*1024];
    int q=stb_easy_font_print(0,0,(char*)t,nullptr,buf,sizeof(buf));
    glEnableClientState(GL_VERTEX_ARRAY); glVertexPointer(2,GL_FLOAT,16,buf);
    if(shadow){ glColor4f(0,0,0,a*0.6f); glPushMatrix(); glTranslatef(x+2,y+2,0); glScalef(s,s,1);
        glDrawArrays(GL_QUADS,0,q*4); glPopMatrix(); }
    glColor4f(r,g,b,a); glPushMatrix(); glTranslatef(x,y,0); glScalef(s,s,1);
    glDrawArrays(GL_QUADS,0,q*4); glPopMatrix(); glDisableClientState(GL_VERTEX_ARRAY);
}
static void key_callback(GLFWwindow* w,int key,int,int action,int){
    if(action==GLFW_PRESS){
        if(key==GLFW_KEY_DOWN) selection=(selection+1)%4;
        else if(key==GLFW_KEY_UP) selection=(selection+3)%4;
        else if(key==GLFW_KEY_ENTER||key==GLFW_KEY_KP_ENTER){confirmed=true;glfwSetWindowShouldClose(w,GL_TRUE);}
        else if(key==GLFW_KEY_ESCAPE){confirmed=false;glfwSetWindowShouldClose(w,GL_TRUE);}
    }
}

int main(){
    if(!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);
    GLFWwindow* window=glfwCreateWindow(900,600,"Select Track",nullptr,nullptr);
    if(!window){glfwTerminate();return-1;}
    glfwMakeContextCurrent(window); glfwSetKeyCallback(window,key_callback); glfwSwapInterval(1);

    while(!glfwWindowShouldClose(window)){
        glfwPollEvents();
        int fbw,fbh; glfwGetFramebufferSize(window,&fbw,&fbh);
        glViewport(0,0,fbw,fbh);
        glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0,fbw,fbh,0,-1,1);
        glMatrixMode(GL_MODELVIEW);  glLoadIdentity();
        glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

        // Background gradient
        glBegin(GL_QUADS);
        glColor3f(0.06f,0.07f,0.10f); glVertex2f(0,0); glVertex2f(fbw,0);
        glColor3f(0.02f,0.02f,0.03f); glVertex2f(fbw,fbh); glVertex2f(0,fbh);
        glEnd();

        // ==== Bigger, window-fitting panel (with dynamic margins) ====
        const float marginX = std::max(24.f, fbw * 0.05f);
        const float marginY = std::max(24.f, fbh * 0.10f);
        float panelW = std::max(480.f, fbw - 2*marginX);
        float panelH = std::max(420.f, fbh - 2*marginY);
        float panelX = 0.5f*(fbw-panelW), panelY = 0.5f*(fbh-panelH);

        drawRect(panelX,panelY,panelW,panelH,1,1,1,0.06f);
        drawOutlineRect(panelX,panelY,panelW,panelH,1,1,1,0.15f,2.f);

        // Scale UI off panel height so everything grows with the panel
        float base       = clampf(panelH / 520.0f, 1.0f, 5.0f);
        float titleScale = clampf(base*2.1f, 2.0f, 5.0f);
        float itemScale  = clampf(base*1.7f, 1.2f, 4.0f);
        float hintScale  = clampf(base*1.2f, 1.0f, 3.0f);
        float pad        = 24.0f * base; // scaled padding

        // Title
        const char* title="COMP371 Racer - Select Track";
        float tw,th; textBounds(title,tw,th);
        float titleW=tw*titleScale, titleH=th*titleScale;
        float titleX=panelX+(panelW-titleW)*0.5f, titleY=panelY+pad;
        drawText(titleX,titleY,titleScale,title,0.95f,0.96f,0.98f,1.0f);

        // Controls block (anchored to bottom)
        const char* controlsTitle="Controls";
        const char* lines[]={
            "Menu: Up/Down to navigate, Enter to start, Esc to quit",
            "Drive: I = throttle, K = brake/reverse; J/L = steer",
            "Cameras: 1 = first-person, 2 = third-person, 3 = chase",
            "Headlights: H   |   Free-cam: W A S D (+ Shift to sprint)"
        }; int nLines=4;

        float ctlTitleW0, ctlTitleH0; textBounds(controlsTitle, ctlTitleW0, ctlTitleH0);
        float lineH   = 16.0f*hintScale + 10.0f*base;
        float ctrlBoxH= (ctlTitleH0*titleScale*0.75f) + 16.0f*base + nLines*lineH + 16.0f*base;
        float boxX = panelX + pad, boxW = panelW - pad*2.f;
        float boxY = panelY + panelH - pad - ctrlBoxH;
        float keepGap = 18.0f * base;

        // Fit menu in remaining space (auto-shrink if needed)
        float topBound    = titleY + titleH + pad;
        float bottomBound = boxY - keepGap;
        float avail       = std::max(bottomBound - topBound, 40.0f);

        float itemHpx = 16.0f*itemScale;
        float needed  = 4*itemHpx;
        if(needed > avail){
            float s = avail / needed;
            itemScale = clampf(itemScale*s, 1.0f, 4.0f);
            itemHpx   = 16.0f*itemScale;
        }
        float vgap   = std::max((avail - 4*itemHpx)/3.0f, 0.0f);
        float listTop= topBound;

        // Menu items
        double t = glfwGetTime();
        for(int i=0;i<4;++i){
            const char* txt=options[i];
            float lw0,lh0; textBounds(txt,lw0,lh0);
            float lw=lw0*itemScale;
            float x = panelX + (panelW - lw)*0.5f;
            float y = listTop + i*(itemHpx + vgap);

            if(i==selection){
                float pulse=0.55f + 0.45f*(float)std::sin(t*4.0);
                float padX=18.f*base, padY=8.f*base;
                float boxWsel= lw + padX*2.f, boxHsel= itemHpx + padY*2.f;
                drawRect(x-padX,y-padY,boxWsel,boxHsel,0.20f,0.60f,1.0f,0.15f*pulse);
                drawOutlineRect(x-padX,y-padY,boxWsel,boxHsel,0.35f,0.75f,1.0f,0.25f);
                drawText(x-padX-(8.f*itemScale)-10.f*base,y,itemScale,">",0.85f,0.95f,1.0f,1.0f,false);
                drawText(x,y,itemScale,txt,0.95f,0.98f,1.0f,1.0f);
            }else{
                drawText(x,y,itemScale,txt,0.78f,0.80f,0.84f,1.0f);
            }
        }

        // Controls block
        drawRect(boxX,boxY,boxW,ctrlBoxH,0,0,0,0.18f);
        drawOutlineRect(boxX,boxY,boxW,ctrlBoxH,1,1,1,0.12f);
        float ctX = panelX + (panelW - ctlTitleW0*titleScale*0.75f)*0.5f;
        float ctY = boxY + 12.0f*base;
        drawText(ctX,ctY,titleScale*0.75f,controlsTitle,0.92f,0.96f,1.0f,1.0f);
        float lineY = ctY + 36.0f*base;
        for(int i=0;i<nLines;++i){
            float lw0,lh0; textBounds(lines[i],lw0,lh0);
            float lw=lw0*hintScale;
            float lx= panelX + (panelW - lw)*0.5f;
            drawText(lx,lineY,hintScale,lines[i],0.84f,0.87f,0.92f,1.0f);
            lineY += lineH;
        }

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    if(!confirmed) return 0;
    switch(selection){
        case 0: std::system("./App"); break;
        case 1: std::system("./App_circularTrack"); break;
        case 2: std::system("./App_night"); break;
        case 3: std::system("./App_raceTrack"); break;
    }
    return 0;
}
