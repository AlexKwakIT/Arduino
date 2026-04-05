// Local Positioning System by Time Difference of Arrival

#include <Arduino.h>
#include <math.h>

// ----- CONFIGURATION -----
const int NUM_MICS = 3;
const int MIC_PINS[NUM_MICS] = {A0, A1, A2};
const float TARGET_FREQ = 1000.0;   // Hz
const float SAMPLE_RATE = 200000.0; // Hz
const int N = 100;                   // Goertzel window
const float THRESHOLD = 5000.0;     // detection threshold
const float SPEED_OF_SOUND = 343.0; // m/s

struct Point { float x; float y; };
// Set your mic positions in meters
const Point MIC_POS[NUM_MICS] = {
  {0.0, 0.0},
  {2.0, 0.0},
  {1.0, 1.5}  // example triangle layout
};

// ----- DMA buffers -----
volatile uint16_t bufferA[NUM_MICS * N];
volatile uint16_t bufferB[NUM_MICS * N];
volatile bool bufferA_full = false;
volatile bool bufferB_full = false;

// ----- Goertzel variables -----
float q0[NUM_MICS], q1[NUM_MICS], q2[NUM_MICS];
float coeff;
float sineCoeff, cosineCoeff;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  for(int i=0;i<NUM_MICS;i++) q0[i]=q1[i]=q2[i]=0;

  // Goertzel coefficient
  float k = 0.5 + (N*TARGET_FREQ/SAMPLE_RATE);
  float omega = 2.0*PI*k/N;
  sineCoeff = sin(omega);
  cosineCoeff = cos(omega);
  coeff = 2.0*cosineCoeff;

  Serial.println("Starting 2D TDOA localization...");

  // ADC Setup
  pmc_enable_periph_clk(ID_ADC);
  ADC->ADC_MR |= ADC_MR_FREERUN_ON | ADC_MR_PRESCAL(1);
  for(int i=0;i<NUM_MICS;i++) ADC->ADC_CHER |= 1<<MIC_PINS[i];

  // DMA ping-pong
  ADC->ADC_RPR = (uint32_t)bufferA;
  ADC->ADC_RCR = NUM_MICS*N;
  ADC->ADC_RNPR = (uint32_t)bufferB;
  ADC->ADC_RNCR = NUM_MICS*N;
  ADC->ADC_PTCR = ADC_PTCR_RXTEN;
  ADC->ADC_CR = ADC_CR_START;
}

void loop() {
  if(bufferA_full){ processBuffer(bufferA); bufferA_full=false; }
  if(bufferB_full){ processBuffer(bufferB); bufferB_full=false; }
}

// ----- Process buffer -----
void processBuffer(volatile uint16_t* buf){
  for(int i=0;i<NUM_MICS;i++) q0[i]=q1[i]=q2[i]=0;

  // Goertzel
  for(int s=0;s<N;s++){
    for(int i=0;i<NUM_MICS;i++){
      float x = buf[s*NUM_MICS+i] - 2048;
      q0[i] = coeff*q1[i] - q2[i] + x;
      q2[i] = q1[i];
      q1[i] = q0[i];
    }
  }

  // Compute magnitude and phase
  float Re[NUM_MICS], Im[NUM_MICS], mag[NUM_MICS], phase[NUM_MICS];
  for(int i=0;i<NUM_MICS;i++){
    Re[i] = q1[i] - q2[i]*cosineCoeff;
    Im[i] = q2[i]*sineCoeff;
    mag[i] = sqrt(Re[i]*Re[i]+Im[i]*Im[i]);
    phase[i] = atan2(Im[i], Re[i]);
  }

  // Skip weak signals
  float maxMag = mag[0];
  for(int i=1;i<NUM_MICS;i++) if(mag[i]>maxMag) maxMag=mag[i];
  if(maxMag<THRESHOLD) return;

  // --- Compute phase differences ---
  float dt12 = phaseDifference(phase[1], phase[0])/(2*PI*TARGET_FREQ);
  float dt13 = phaseDifference(phase[2], phase[0])/(2*PI*TARGET_FREQ);

  // Convert to distance differences
  float dd12 = SPEED_OF_SOUND * dt12;
  float dd13 = SPEED_OF_SOUND * dt13;

  // --- Solve nonlinear 2D hyperbola intersection (approx via iterative linearization) ---
  Point P = solve2D(MIC_POS[0], MIC_POS[1], MIC_POS[2], dd12, dd13);

  Serial.print("Source approx at (m): x=");
  Serial.print(P.x,3);
  Serial.print(" y=");
  Serial.println(P.y,3);
}

// ----- Helper: phase difference with unwrapping -----
float phaseDifference(float phi2, float phi1){
  float diff = phi2 - phi1;
  while(diff > PI) diff -= 2*PI;
  while(diff < -PI) diff += 2*PI;
  return diff;
}

// ----- Linearized solver for 3 mic TDOA in 2D -----
Point solve2D(Point m0, Point m1, Point m2, float dd12, float dd13){
  // distance difference: |P-m1| - |P-m0| = dd12
  //                       |P-m2| - |P-m0| = dd13
  // Linearize using initial guess at center
  float Px= (m0.x + m1.x + m2.x)/3.0;
  float Py= (m0.y + m1.y + m2.y)/3.0;

  for(int iter=0; iter<5; iter++){
    float r0 = sqrt((Px-m0.x)*(Px-m0.x)+(Py-m0.y)*(Py-m0.y));
    float r1 = sqrt((Px-m1.x)*(Px-m1.x)+(Py-m1.y)*(Py-m1.y));
    float r2 = sqrt((Px-m2.x)*(Px-m2.x)+(Py-m2.y)*(Py-m2.y));

    float f1 = r1 - r0 - dd12;
    float f2 = r2 - r0 - dd13;

    // partial derivatives
    float df1dx = (Px-m1.x)/r1 - (Px-m0.x)/r0;
    float df1dy = (Py-m1.y)/r1 - (Py-m0.y)/r0;
    float df2dx = (Px-m2.x)/r2 - (Px-m0.x)/r0;
    float df2dy = (Py-m2.y)/r2 - (Py-m0.y)/r0;

    float det = df1dx*df2dy - df2dx*df1dy;
    if(fabs(det)<1e-6) break; // singular, stop

    float dx = ( f1*df2dy - f2*df1dy)/det;
    float dy = (-f1*df2dx + f2*df1dx)/det;

    Px -= dx;
    Py -= dy;

    if(fabs(dx)<1e-6 && fabs(dy)<1e-6) break; // convergence
  }

  Point result = {Px, Py};
  return result;
}

// ----- ADC Handler -----
void ADC_Handler(){
  uint32_t status=ADC->ADC_ISR;
  if(status & ADC_ISR_ENDRX) bufferA_full=true;
  if(status & ADC_ISR_ENDRX) bufferB_full=true;
}