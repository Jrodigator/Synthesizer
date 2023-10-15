#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "./include/raylib.h"

#include "./include/synthesizer.h"

#define RAYGUI_IMPLEMENTATION
#include "./include/raygui.h"  

#define SAMPLE_RATE 44100
#define STREAM_BUFFER_SIZE 1024
#define NO_OSCILLATORS 3

// TODO:
// 1. Add more wave types
// 2. Add more oscillators
// 3. Add Filters 
//    -> low-pass filter 
//    -> way better forrier transform implementation

// RESTRUCTURE 
// 
//  A SINGLE UNIT 
//   - Envolope Generator -> LFO (low Frequency Oscilator) -> VCO (Voltage Controlled Oscilator) >> piped into VCA below 
//   - Envolope Generator -> VCA (Voltage Controlled Amplifier) >> Piped into VCF below
//   - Envolope Generator -> VCF (Voltage Controlled Filter)
//
//  VCO -> takes in pitch as parameter (freqency) & Wave Type
//  VCA -> takes in amplitude/velocity as parameter (volume)
//  VCF -> takes in frequency as parameter (cutoff frequency) & Q factor (resonance)
//
// Multiple Units Combined together into master unit where it can be further filtered.

// Functions
//--------------------------------------------------------------------------------------



void accumulateSignal(float *signal, Oscillator *osc, float amplitude, int wave_type, float envolope) {
    
    if (wave_type == 1) {
        printf("Sine wave\n amplitude : %f\n", envolope);
        for (size_t t = 0; t < STREAM_BUFFER_SIZE; t++) {
            updateOsc(osc);
            signal[t] += sinf(2.0f * PI * osc->phase) * amplitude;
            signal[t] = signal[t] * envolope;
        }
    } else if (wave_type == 2) {
        printf("Square wave\n amplitude : %f\n", envolope);
        for (size_t t = 0; t < STREAM_BUFFER_SIZE; t++) {
            updateOsc(osc);
            signal[t] += osc->phase * (((int)(t) % 60) < 30 ? amplitude : -amplitude); // sinf(2.0f * PI * osc->phase) * amplitude;
            signal[t] = signal[t] * envolope;
        }
    } else if (wave_type == 3) {
        for (size_t t = 0; t < STREAM_BUFFER_SIZE; t++) {
            updateOsc(osc);
            if (osc->phase < 0.5f)
                signal[t] = (osc->phase * 4.0f) - 1.0f;
            else
                signal[t] = (osc->phase * -4.0f) + 3.0f;
            signal[t] = signal[t] * envolope;
        }
    }
}


//--------------------------------------------------------------------------------------
// Main entry point
//--------------------------------------------------------------------------------------

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    // screen & window
    const int screenWidth = 1800; //1024;
    const int screenHeight = 900; //768;
    InitWindow(screenWidth, screenHeight, "Synthesizer");
    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------
    // audio
    unsigned int sampleRate = SAMPLE_RATE;
    float sampleRange = 1.0f / sampleRate;
    InitAudioDevice();
    //SetMasterVolume(1.0f);
    SetAudioStreamBufferSizeDefault(STREAM_BUFFER_SIZE); // number of samples

    AudioStream stream = LoadAudioStream(sampleRate, 32, 1);
    SetAudioStreamVolume(stream, 0.2f); //0.2f);
    //SetAudioStreamPitch(stream, 1.0f);
    PlayAudioStream(stream);


    // Oscillator
    //----------------------------------------------------------------------------------    
    float signal[STREAM_BUFFER_SIZE]; 
    float signal1[STREAM_BUFFER_SIZE];
    float signal2[STREAM_BUFFER_SIZE];
    float signal3[STREAM_BUFFER_SIZE];

    float frequency = 0.0f;
    float detune = 0.1f;
    float baseFrequency = 440.0f * (1.0f/NO_OSCILLATORS) * detune;
    float fp = 0.0f;
    float amp_osc = 1.0f / NO_OSCILLATORS;
    Vector2 mousePosition = { -100.0f, -100.0f };

    Oscillator osc[NO_OSCILLATORS] = {0}; //{ 0.0f , frequency * (1.0f / sampleRate), frequency};

    bool sig1 = false;
    int wave_typeA = 1;
    int modCountA = 1;

    bool sig2 = true;
    int wave_typeB = 2;
    int modCountB = 3;

    bool sig3 = true;
    int wave_typeC = 3;
    int modCountC = 9;

    int envTrack = 1;

    float pan = 0.5f;
    float masterVolume = 0.2f;
    float noteFrequency = 277.18f;
    
    Envolope envolopes[3];
    for (int i = 0; i < 3; i++) {
        envolopes[i].attack = 25.0f;
        envolopes[i].decay = 50.0f;
        envolopes[i].sustain = 0.5f; 
        envolopes[i].release = 100.0f;
        envolopes[i].amplitude = 0.0f;
    }
    Envolope *envolope = &envolopes[0];
    
    // KeyNotes
    KeyNotes* Notes = InitNotes();
    char NoteFlag = 'A'; 
  
    clock_t start, interval; 
    start = clock();
    

    //----------------------------------------------------------------------------------
    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        
        mousePosition = GetMousePosition();

        interval = clock() - start; //DeltaTime 
        //printf("Interval: %f\n", (float)interval);


        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            fp = (float)(mousePosition.y) / (float) screenHeight;
            //pan = (float)(mousePosition.x) / (float)screenWidth;
            //SetAudioStreamPan(stream, pan);
        }
        
        
        // Audio 
        //----------------------------------------------------------------------------------
        detune = 1.0f + fp * 10.0f;

        updateEnv(&envolopes[0], interval); 
        updateEnv(&envolopes[1], interval);
        updateEnv(&envolopes[2], interval);

        if (IsAudioStreamProcessed(stream)) {
            
            zeroSignal(signal, STREAM_BUFFER_SIZE);
            zeroSignal(signal1, STREAM_BUFFER_SIZE);
            zeroSignal(signal2, STREAM_BUFFER_SIZE);
            zeroSignal(signal3, STREAM_BUFFER_SIZE);

            // for (size_t i = 0; i < NO_OSCILLATORS; i++) {

            //     //baseFrequency = 50.0f + detune*70.0f; //440.0f * (1.0f/NO_OSCILLATORS) * detune;
            //     baseFrequency = noteFrequency;
            //     frequency = baseFrequency * (i) ;
            //     float shift = frequency * sampleRange ;
            //     osc[i].phase_shift = shift;
            //     accumulateSignal(signal1, &osc[i], amp_osc, wave_typeA, envolope.amplitude);
            //     //accumulateSignal(signal2, &osc[i], amp_osc, wave_typeB, envolope.amplitude);
            //     accumulateSignal(signal3, &osc[i], amp_osc, wave_typeC, envolope.amplitude);

            //     //printf("Wave type: %d\n", wave_typeA);
            // }
            if (sig1) {
                for (size_t i = 0; i < modCountA; i++) {
                    float shift = sampleRange * noteFrequency * (i+1);
                    osc[i].phase_shift = shift;
                    accumulateSignal(signal1, &osc[i], 1 / (float)modCountA, wave_typeA, envolopes[0].amplitude);
                }
            }
            if (sig2) {
                for (size_t i = 0; i < modCountB; i++) {
                    float shift = sampleRange * noteFrequency * (i+1);
                    osc[i].phase_shift = shift;
                    accumulateSignal(signal2, &osc[i], 1 / (float)modCountB, wave_typeB, envolopes[1].amplitude);
                }
            }
            if (sig3) {
                for (size_t i = 0; i < modCountC; i++) {
                    float shift = sampleRange * noteFrequency * (i+1);
                    osc[i].phase_shift = shift;
                    accumulateSignal(signal3, &osc[i], 1 / (float)modCountC, wave_typeC, envolopes[2].amplitude);
                }
            }

            if (sig1 | sig2 | sig3) {
                for (size_t i = 0; i < STREAM_BUFFER_SIZE; i++) {
                    signal[i] = (signal1[i]*sig1  + signal2[i]*sig2 + signal3[i]*sig3 )/(sig1 + sig2 + sig3);
                }
            }
            UpdateAudioStream(stream, signal, STREAM_BUFFER_SIZE);
            SetAudioStreamPan(stream, 1.0f - pan);
            SetAudioStreamVolume(stream, masterVolume);
        }    
        if (!sig1 && !sig2 && !sig3) {PauseAudioStream(stream);} else {ResumeAudioStream(stream);}

        if (IsKeyDown(49)) {sig1 = !sig1;} // 48 -> 57 (0 -> 9)
        if (IsKeyDown(50)) {sig2 = !sig2;}
        if (IsKeyDown(51)) {sig3 = !sig3;}

        if (IsKeyDown(KEY_A)) { NoteFlag = 'A'; noteFrequency = 277.18f; start = clock();}
        if (IsKeyDown(KEY_S)) { NoteFlag = 'S'; noteFrequency = 311.13f; start = clock();}
        if (IsKeyDown(KEY_D)) { NoteFlag = 'D'; noteFrequency = 369.99f; start = clock();}
        if (IsKeyDown(KEY_F)) { noteFrequency = 415.30f; start = clock();}
        if (IsKeyDown(KEY_G)) { noteFrequency = 466.16f; start = clock();}   
        if (IsKeyDown(KEY_H)) { noteFrequency = 554.37f; start = clock();}
        if (IsKeyDown(KEY_J)) { noteFrequency = 622.25f; start = clock();}
        if (IsKeyDown(KEY_K)) { noteFrequency = 739.99f; start = clock();}
        if (IsKeyDown(KEY_L)) { noteFrequency = 830.61f; start = clock();}
        if (IsKeyDown(KEY_Z)) { noteFrequency = 932.33f; start = clock();}
        if (IsKeyDown(KEY_X)) { noteFrequency = 1108.73f; start = clock();}
        if (IsKeyDown(KEY_C)) { noteFrequency = 1244.51f; start = clock();}
        if (IsKeyDown(KEY_V)) { noteFrequency = 1479.98f; start = clock();}
        if (IsKeyDown(KEY_B)) { noteFrequency = 1661.22f; start = clock();}
        if (IsKeyDown(KEY_N)) { noteFrequency = 1864.66f; start = clock();}
        if (IsKeyDown(KEY_M)) { noteFrequency = 2217.46f; start = clock();}

        
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();
        // GUI 
        ClearBackground(BLACK);
        DrawLine(0, screenHeight/5.0f, screenWidth, screenHeight/5.0f, Fade(LIGHTGRAY, 0.6f)); //TOP Section
        DrawRectangle(0, 0, screenWidth, screenHeight/5.0f, Fade(GRAY, 0.3f)); // Top Container

        DrawLine(screenWidth * 3/4, screenHeight/5.0f, screenWidth * 3/4, screenHeight, Fade(LIGHTGRAY, 0.6f)); //Right Section
        DrawRectangle(screenWidth * 3/4, screenHeight/5.0f, screenWidth * 1/4, screenHeight * 4/5.0f, Fade(GRAY, 0.3f)); // right side container

        float line1 = screenHeight/5.0f * 2.0f - screenHeight/5.0f * 0.8f * (1.0f - sig1);
        float line2 = screenHeight/5.0f * 3.0f - screenHeight/5.0f * 0.8f * (2.0f - sig2 - sig1);
        float line3 = screenHeight/5.0f * 4.0f - screenHeight/5.0f * 0.8f * (3.0f - sig3 - sig2 - sig1);
        
        DrawLine(0, line1, screenWidth, line1, Fade(LIGHTGRAY, 0.6f)); //End of osc 1
        DrawLine(0,line2, screenWidth, line2, Fade(LIGHTGRAY, 0.6f)); //End of osc 2
        // Display Keys Currently Pressed 
        GetCurrentKeys(Notes);
        for(int i = 0; i < 16; i++ ) {
            if (Notes[i].isPressed) {
                DrawText(TextFormat("%c", Notes[i].keycode), 250 + 20 * i, 10 + 10, 30, RED);
            } else {
                //DrawRectangle(0, screenHeight/5.0f * (i+1), screenWidth, screenHeight/5.0f, Fade(GRAY, 0.3f));
                DrawText(TextFormat("%c", Notes[i].keycode), 250 + 20 * i, 10 + 10, 25, BLUE);
            } 
        } 

        DrawLine(0, line3, screenWidth, line3, Fade(LIGHTGRAY, 0.6f)); //End of osc 3
        //----------------------------------------------------------------------------------
        // Global Settings
        //----------------------------------------------------------------------------------
        pan = GuiSliderBar((Rectangle){ 40, 120, 120, 20 }, "Pan", NULL, pan, 0.0f, 1.0f);                         //PAN
        masterVolume = GuiSliderBar((Rectangle){ 40, 150, 120, 20 }, "Volume", NULL, masterVolume, 0.0f, 1.0f); //VOLUME
        
        DrawText("JROD SYNTHESIZER", 10, 10, 20, RED);
        DrawText(TextFormat("Frequency:\t%10.0f", frequency)  , 10, 30, 20, RED);
        DrawText(TextFormat("Is streaming:\t%s", "true")  , 10, 50, 20, RED);
        
        DrawText(TextFormat("Last Note : %c", NoteFlag) , 10, 70, 20, RED);
        DrawFPS(10, screenHeight - 30);

        //----------------------------------------------------------------------------------
        // OSC 1, 2, 3
        //----------------------------------------------------------------------------------
        DrawText("OSC 1", 10, screenHeight/5.0f + 10, 20, RED);
        DrawText("OSC 2", 10, line1 + 10, 20, GREEN);
        DrawText("OSC 3", 10, line2 + 10, 20, BLUE);
        
        // Wave Parameters 
        if (sig1) {
            // Wave Type
            if (GuiButton((Rectangle){ screenWidth * 4/5 + 10, screenHeight/5.0f + 5, 120, 20 }, "Wave")) {
                wave_typeA = (wave_typeA ) % 3 + 1;
            }
            DrawText(TextFormat(" \t%d", wave_typeA), screenWidth * 4/5 + 120, screenHeight/5.0f + 5, 20, RED);
            // Modulation
            if (GuiButton((Rectangle){ screenWidth * 4/5 + 160, screenHeight/5.0f + 5, 120, 20 }, ("Mod"))) {
                modCountA = (modCountA) % 20 + 2;
            }
            DrawText(TextFormat(" \t%d", modCountA), screenWidth * 4/5 + 280, screenHeight/5.0f + 5, 20, RED);
        }
        if (sig2) {
            // wave type
            if (GuiButton((Rectangle){ screenWidth * 4/5, line1 + 5, 120, 20 }, "Wave")) {
                wave_typeB = (wave_typeB) % 3 + 1;
            }
            DrawText(TextFormat(" \t%d", wave_typeB), screenWidth * 4/5 + 120, line1 + 5, 20, GREEN);
            // Modulation
            if (GuiButton((Rectangle){ screenWidth * 4/5 + 160, line1 + 5, 120, 20 }, ("Mod"))) {
                modCountB = (modCountB) % 20 + 2;
            }
            DrawText(TextFormat(" \t%d", modCountB), screenWidth * 4/5 + 280, line1 + 5, 20, GREEN);

        }
        if (sig3) {
            if (GuiButton((Rectangle){ screenWidth * 4/5 + 10, line2 + 5, 120, 20 }, "Wave")) {
                wave_typeC = (wave_typeC) % 3 + 1;
            }
            DrawText(TextFormat(" \t%d", wave_typeC), screenWidth * 4/5 + 120, line2 + 5, 20, BLUE);
            // Modulation
            if (GuiButton((Rectangle){ screenWidth * 4/5 + 160, line2 + 5, 120, 20 }, ("Mod"))) {
                modCountC = (modCountC) % 20 + 2;
            }
            DrawText(TextFormat(" \t%d", modCountC), screenWidth * 4/5 + 280, line2 + 5, 20, BLUE);

        }
        // Wave Modulation



        sig1 = GuiCheckBox((Rectangle){ screenWidth * 3/4 + 5, screenHeight * 1/5 + 5, 20, 20 }, "ENDABLED", sig1);
        sig2 = GuiCheckBox((Rectangle){ screenWidth * 3/4 + 5, line1 + 5, 20, 20 }, "ENDABLED", sig2);
        sig3 = GuiCheckBox((Rectangle){ screenWidth * 3/4 + 5, line2 + 5, 20, 20 }, "ENDABLED", sig3);
        //----------------------------------------------------------------------------------
        // ENVOLOPE
        //----------------------------------------------------------------------------------
        GuiGroupBox((Rectangle){ screenWidth * 3/4 + 5, screenHeight * 4/5 + 8, screenWidth * 1/4 - 10, screenHeight * 1/5 - 10 }, "ENVOLOPE");
        envolope->attack = GuiSliderBar((Rectangle){ screenWidth * 3/4 + 70, screenHeight * 4/5 + 30, screenWidth * 1/4 - 80, 20 }, "Attack ", NULL, envolope->attack, 0.0f, 100.0f);
        envolope->decay = GuiSliderBar((Rectangle){ screenWidth * 3/4 + 70, screenHeight * 4/5 + 60, screenWidth * 1/4 - 80, 20 }, "Decay  ", NULL, envolope->decay, envolope->attack, envolope->attack + 200.0f);
        envolope->sustain = GuiSliderBar((Rectangle){ screenWidth * 3/4 + 70, screenHeight * 4/5 + 90, screenWidth * 1/4 - 80, 20 }, "Sustain ", NULL, envolope->sustain, 0.0f, 1.0f);
        envolope->release = GuiSliderBar((Rectangle){ screenWidth * 3/4 + 70, screenHeight * 4/5 + 120, screenWidth * 1/4 - 80, 20 }, "Release ", NULL, envolope->release, 0.0f, 1000.0f);
        // select which osc to apply envolope to
        if (GuiButton((Rectangle){ screenWidth * 3/4 + 70, screenHeight * 4/5 + 150, 80, 20 }, "Switch")) {
            // apply envolope to seperate osc
            envTrack = (envTrack) % 3 + 1;
            envolope = &envolopes[envTrack - 1];
        }
        DrawText(TextFormat(" \tEnv->Osc: %d", envTrack), screenWidth * 3/4 + 150, screenHeight * 4/5 + 150, 20, RED);
        //----------------------------------------------------------------------------------



        // if (GuiButton((Rectangle){ screenWidth - 170, screenHeight - 50, 150, 30 }, "OSC1")) {
        //     sig1 = !sig1;
        //     if (sig1) {str1 = "ENABLE";} else {str1 = "DISABLE";}
        // }

        //----------------------------------------------------------------------------------

        


        // Osciliscope 
        for (size_t i = 0; i < STREAM_BUFFER_SIZE; i++) {
            //signal 1 
            if(sig1) DrawPixel(i * (screenWidth * 3/4) / STREAM_BUFFER_SIZE, (screenHeight/5.0f + screenHeight/5 * 0.5) + (int)(signal1[i] * 200), RED);
            if(sig2) DrawPixel(i * (screenWidth * 3/4) / STREAM_BUFFER_SIZE, line1 + (line2 - line1)/2 + (int)(signal2[i] * 200), GREEN);
            if(sig3) DrawPixel(i * (screenWidth * 3/4) / STREAM_BUFFER_SIZE, line2 + (line3 - line2)/2 + (int)(signal3[i] * 200), BLUE);
        }

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    if (Notes != NULL) {free(Notes); Notes = NULL;}
    UnloadAudioStream(stream);
    CloseAudioDevice();
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}