#include <stdio.h>
#include <math.h>
#include <stdint.h>
//#include <memory.h>
#include <detection.h>

#define GLOBAL_IQ 1
#include "IQmathLib.h"

uint8_t *spectrum0;
uint8_t *spectrum1;
uint8_t *spectrum2;
uint8_t aggregate[288];
uint32_t HPS[88];

uint32_t thresholds[66] = {
    725,
    760,
    798,
    841,
    888,
    940,
    997,
    1061,
    1132,
    1210,
    1296,
    1392,
    1497,
    1615,
    1744,
    1888,
    2046,
    2222,
    2416,
    2631,
    2869,
    3133,
    3424,
    3747,
    4104,
    4499,
    4937,
    5421,
    5957,
    6549,
    7205,
    7931,
    8735,
    9624,
    10200,
    10608,
    12284,
    15233,
    18889,
    23424,
    29046,
    36019,
    44665,
    55387,
    68682,
    85169,
    105613,
    130965,
    162402,
    201386,
    249728,
    309674,
    384010,
    476190,
    590497,
    732243,
    908014,
    1125978,
    1396264,
    1731430,
    2147052,
    2662441,
    3301547,
    4094067,
    5076829,
    6295497
};

char output[6][3];
char binToMidi[128] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    40,
    41,
    42,
    43,
    44,
    45,
    46,
    47,
    48,
    49,
    50,
    50,
    51,
    52,
    52,
    53,
    54,
    54,
    55,
    55,
    56,
    57,
    58,
    59,
    60,
    61,
    62,
    62,
    63,
    64,
    64,
    65,
    66,
    66,
    67,
    67,
    68,
    69,
    70,
    71,
    72,
    73,
    74,
    74,
    75,
    76,
    76,
    77,
    78,
    78,
    79,
    79,
    80,
    80,
    81,
    81,
    82,
    82,
    83,
    83,
    84,
    84,
    84,
    85,
    85,
    86,
    86,
    86,
    87,
    87,
    87,
    88,
    88,
    88,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};

uint32_t rms = 0;

uint32_t magFloor(uint32_t mag){
    if(mag > 0){
        return mag;
    }else{
        return 1;
    }
}

void generateAggregateSpec(void){
    int i;
    char temp;

    temp = spectrum0[0];
    for(i = 1; i < 32; i++){
        spectrum0[i-1] = spectrum0[i];
    }
    spectrum0[31] = temp;

    temp = spectrum1[0];
    for(i = 1; i < 32; i++){
        spectrum1[i-1] = spectrum1[i];
    }
    spectrum1[31] = temp;

    temp = spectrum2[0];
    for(i = 1; i < 256; i++){
        spectrum2[i-1] = spectrum2[i];
    }
    spectrum2[255] = temp;

    for(i = 0; i < 32; i++){
        aggregate[i] = spectrum0[i];
    }

    for(i = 32; i < 48; i++){
        aggregate[i] = spectrum1[i-16];
    }

    for(i = 48; i < 64; i++){
        aggregate[i] = spectrum2[i-32];
    }

    for(i = 64; i < 287; i++){
        //aggregate[i] =  spectrum2[i-32] << 1;
        aggregate[i] =  spectrum2[i-32];
    }

    int max = 16;

    for(i = 0; i < 256; i++){
        if(aggregate[i] > max){
            max = aggregate[i];
        }
    }

    int shiftNum = 0;

    if(max != 16){
        while(max < 70){
            max = max << 1;
            shiftNum++;
        }
        for(i = 0; i < 256;i++){
            aggregate[i] = aggregate[i] << shiftNum;
        }
    }
}

void generateHPS(void){
    int i;

    for(i = 0; i < 88; i++){
        HPS[i] = aggregate[i];
    }

    int bin = 11;

    for(i = 23; i < 32; i+=2){
        HPS[bin] *= magFloor(aggregate[i]);
        bin++;
    }

    bin = 11;
    int div = 0;
    for(i = 24; i < 32; i+=2){
       HPS[bin] *= magFloor(aggregate[i] >> 2);
       HPS[bin+1] *= magFloor(aggregate[i] >> 2);
       bin++;
    }

    for(i = 32; i < 61; i++){
        HPS[i-16] *= magFloor(aggregate[i]);
    }
    for(i = 61; i < 64; i++){
        HPS[i-16] *= magFloor(aggregate[i] << 1);
    }

    bin = 48;

    for(i = 65; i < 72; i+=2){
        HPS[bin] *= magFloor(aggregate[i] << 1);
        bin++;
    }
    for(i = 73; i < 98; i+=2){
        HPS[bin] *= magFloor(aggregate[i] << 2);
        bin++;
    }
    for(i = 99; i < 145; i+=2){
        HPS[bin] *= magFloor(aggregate[i] << 4);
        bin++;
    }

    bin = 47;

    for(i = 64; i < 73; i+=2){
       HPS[bin] *= magFloor(aggregate[i] >> 1);
       HPS[bin+1] *= magFloor(aggregate[i] >> 1);
       bin++;
    }
    for(i = 74; i < 99; i+=2){
       HPS[bin] *= magFloor(aggregate[i]);
       HPS[bin+1] *= magFloor(aggregate[i]);
       bin++;
    }
    for(i = 100; i < 144; i+=2){
       HPS[bin] *= magFloor(aggregate[i] << 2);
       HPS[bin+1] *= magFloor(aggregate[i] << 2);
       bin++;
    }

    for(i = 12; i < 15;i++){
        //HPS[i] >> 10;
        HPS[i] = magFloor(HPS[i] >> 4);
    }

    HPS[15] = magFloor(HPS[15] >> 1);
    HPS[47] = magFloor(HPS[47] >> 1);

    for(i = 48; i < 88; i++){
        HPS[i] = magFloor(HPS[i] >> 2);
    }

    for(i = 0; i < 36; i++){
        HPS[i] = magFloor(HPS[i] >> 2);
    }

    for(i = 36; i < 88; i++){
        HPS[i] = magFloor(HPS[i] >> 1);
    }

     for(i = 1; i < 88; i++){
        HPS[i] = magFloor(HPS[i]  >> 1);
     }

    bin = 11;
    for(i = 33; i < 48; i+=3){
        HPS[bin] *= magFloor(aggregate[i]);
        HPS[bin] *= magFloor(aggregate[i+1] >> 4);
        HPS[bin+1] *= magFloor(aggregate[i+1] >> 1);
        HPS[bin+1] *= magFloor(aggregate[i+2] >> 1);
        HPS[bin+2] *= magFloor(aggregate[i+2] >> 4);
        bin += 2;

    }

    bin = 48;

    for(i = 22; i < 27; i+=4){
        HPS[i-1] *= magFloor(aggregate[bin] >> 4);
        HPS[i] *= magFloor(aggregate[bin] >> 1);
        HPS[i+1] *= magFloor(aggregate[bin+1] >>1);
        HPS[i+2] *= magFloor(aggregate[bin+2] >> 1);
        HPS[i+3] *= magFloor(aggregate[bin+2] >> 4);
        bin += 3;
    }

    HPS[29] *= magFloor(aggregate[bin] >> 3);
    HPS[30] *= magFloor(aggregate[bin]);
    HPS[31] *= magFloor(aggregate[bin+1]);

    bin = 32;

    for(i = 57; i < 67; i+=3){
        HPS[bin] *= magFloor(aggregate[i]);
        HPS[bin+1] *= magFloor(aggregate[i] >> 3);
        HPS[bin+1] *= magFloor(aggregate[i+1] << 1);
        HPS[bin+1] *= magFloor(aggregate[i+2] >> 3);
        HPS[bin+2] *= magFloor(aggregate[i+2]);
        bin += 2;
    }

    for(i = 69; i < 76; i+=3){
        HPS[bin] *= magFloor(aggregate[i] << 1);
        HPS[bin+1] *= magFloor(aggregate[i] >> 2);
        HPS[bin+1] *= magFloor(aggregate[i+1] << 2);
        HPS[bin+1] *= magFloor(aggregate[i+2] >> 2);
        HPS[bin+2] *= magFloor(aggregate[i+2] << 1);
        bin += 2;
    }

    HPS[46] *= magFloor(aggregate[78] << 1);
    HPS[47] *= magFloor(aggregate[78] >> 2);
    HPS[47] *= magFloor(aggregate[79] << 2);
    HPS[47] *= magFloor(aggregate[80] << 1);
    HPS[48] *= magFloor(aggregate[80] >> 2);

    bin = 48;

    for(i = 81; i < 91; i+=3){
            HPS[bin-1] *= magFloor(aggregate[i] >> 2);
            HPS[bin] *= magFloor(aggregate[i] << 1);
            HPS[bin] *= magFloor(aggregate[i+1] << 2);
            HPS[bin] *= magFloor(aggregate[i+2] << 1);
            HPS[bin+1] *= magFloor(aggregate[i+2] >> 2);
            bin++;
    }

    for(i = 93; i < 198; i+=3){
            HPS[bin-1] *= magFloor(aggregate[i] >> 1);
            HPS[bin] *= magFloor(aggregate[i] << 2);
            HPS[bin] *= magFloor(aggregate[i+1] << 3);
            HPS[bin] *= magFloor(aggregate[i+2] << 2);
            HPS[bin+1] *= magFloor(aggregate[i+2] >> 1);
            bin++;
    }

    for(i = 12; i < 21;i++){
        HPS[i] = magFloor(HPS[i] >> 4);
    }

    for(i = 0; i < 36; i++){
        HPS[i] = magFloor(HPS[i] >> 2);
    }

    /*for(i = 36; i < 88; i++){
        HPS[i] = magFloor(HPS[i] >> 1);
    }*/

    for(i = 32; i < 47; i++){
        HPS[i] = magFloor(HPS[i] >> 3);
    }

    HPS[47] = magFloor(HPS[47] >> 5);

    for(i = 48; i < 88; i++){
        HPS[i] = magFloor(HPS[i] >> 11);
    }

    for(i = 1; i < 88; i++){
        HPS[i] = magFloor(HPS[i]  >> 1);
    }

    bin = 11;
    for(i = 39; i < 48; i+=2){
        HPS[bin] *= magFloor(aggregate[i]);
        bin++;
    }

    bin = 11;

    for(i = 40; i < 47; i+=2){
       HPS[bin] *= magFloor(aggregate[i] >> 2);
       HPS[bin+1] *= magFloor(aggregate[i] >> 2);
       bin++;
    }

    for(i = 48; i < 60; i ++){
        HPS[i-32] *= magFloor(aggregate[i]);
    }

    for(i = 60; i < 64; i ++){
        HPS[i-32] *= magFloor(aggregate[i] << 1);
    }

    bin = 32;
    for(i = 65; i < 86; i +=2){
        HPS[bin] *= magFloor(aggregate[i] << 1);
        bin++;
    }

    for(i = 87; i < 95; i +=2){
        HPS[bin] *= magFloor(aggregate[i] << 3);
        bin++;
    }


    bin = 31;
    for(i = 64; i < 85; i+=2){
       HPS[bin] *= magFloor(aggregate[i] >> 1);
       HPS[bin+1] *= magFloor(aggregate[i] >> 1);
       bin++;
    }
    for(i = 86; i < 95; i+=2){
       HPS[bin] *= magFloor(aggregate[i] << 1);
       HPS[bin+1] *= magFloor(aggregate[i] << 1);
       bin++;
    }

    HPS[47] *= magFloor(aggregate[95] << 3);
    HPS[47] *= magFloor(aggregate[96] << 2);
    HPS[48] *= magFloor(aggregate[96] >> 1);

    bin = 48;
    for(i = 98; i < 252; i+=4){
        HPS[bin-1] *= magFloor(aggregate[i] >> 1);
        HPS[bin] *= magFloor(aggregate[i] << 2);
        HPS[bin] *= magFloor(aggregate[i+1] << 3);
        HPS[bin] *= magFloor(aggregate[i+2] << 2);
        HPS[bin+1] *= magFloor(aggregate[i+2] >> 1);
        bin++;
    }

    bin = 47;
    for(i = 97; i < 251; i+=4){
       HPS[bin] *= magFloor(aggregate[i] << 1);
       HPS[bin+1] *= magFloor(aggregate[i] << 1);
       bin++;
    }

    for(i = 12; i < 15;i++){
        HPS[i] = magFloor(HPS[i] >> 3);
    }

    for(i = 15; i < 17;i++){
        HPS[i] = magFloor(HPS[i] >> 2);
    }

    for(i = 0; i < 36; i++){
        HPS[i] = magFloor(HPS[i] >> 2);
    }

    /*for(i = 36; i < 88; i++){
        HPS[i] = magFloor(HPS[i] >> 1);
    }*/

    HPS[47] = magFloor(HPS[47] >> 6);

    for(i = 48; i < 88; i++){
        HPS[i] = magFloor(HPS[i] >> 10);
    }

    for(i = 1; i < 88; i++){
        HPS[i] = magFloor(HPS[i]  >> 1);
    }

    bin = 10;
    for(i = 42; i < 45; i+=5) {
        HPS[bin] *= magFloor(aggregate[i] >> 1);
        HPS[bin] *= magFloor(aggregate[i+1] >> 1);
        bin++;
        HPS[bin-1] *= magFloor(aggregate[i+2] >> 2);
        HPS[bin] *= magFloor(aggregate[i+2] >> 2);
        HPS[bin] *= magFloor(aggregate[i+3]);
        HPS[bin] *= magFloor(aggregate[i+4] >> 2);
        HPS[bin+1] *= magFloor(aggregate[i+4] >> 2);
        bin++;
    }

    HPS[12] *= magFloor(aggregate[47] >> 1);

    bin = 13;
    for(i = 48; i < 66; i+=5){
        HPS[bin-1] *= magFloor(aggregate[i] >> 2);
        HPS[bin] *= magFloor(aggregate[i] >> 2);
        HPS[bin] *= magFloor(aggregate[i+1] >> 2);
        HPS[bin+1] *= magFloor(aggregate[i+1] >> 2);
        HPS[bin+1] *= magFloor(aggregate[i+2] >> 1);
        HPS[bin+2] *= magFloor(aggregate[i+2] >> 5);
        HPS[bin+2] *= magFloor(aggregate[i+3]);
        HPS[bin+2] *= magFloor(aggregate[i+4] >> 5);
        HPS[bin+3] *= magFloor(aggregate[i+4] >> 1);
        bin += 4;
    }

    HPS[28] *= magFloor(aggregate[68] >> 2);
    HPS[29] *= magFloor(aggregate[68] >> 2);
    HPS[29] *= magFloor(aggregate[69] >> 2);
    HPS[30] *= magFloor(aggregate[69] >> 2);
    HPS[30] *= magFloor(aggregate[70] >> 1);
    HPS[31] *= magFloor(aggregate[70] >> 5);
    HPS[31] *= magFloor(aggregate[71]);
    HPS[31] *= magFloor(aggregate[72] >> 2);
    HPS[32] *= magFloor(aggregate[72] >> 2);

    bin = 32;
    for(i = 73; i < 111; i+=5){
        HPS[bin-1] *= magFloor(aggregate[i] >> 5);
        HPS[bin] *= magFloor(aggregate[i] >> 1);
        HPS[bin] *= magFloor(aggregate[i+1] >> 1);
        HPS[bin+1] *= magFloor(aggregate[i+1] >> 5);
        bin++;
        HPS[bin-1] *= magFloor(aggregate[i+2] >> 2);
        HPS[bin] *= magFloor(aggregate[i+2] >> 2);
        HPS[bin] *= magFloor(aggregate[i+3]);
        HPS[bin] *= magFloor(aggregate[i+4] >> 2);
        HPS[bin+1] *= magFloor(aggregate[i+4] >> 2);
        bin++;
    }

    HPS[47] *= magFloor(aggregate[113] >> 2);
    HPS[48] *= magFloor(aggregate[113] >> 2);

    bin = 48;
    for(i = 114; i < 282; i+=5){
        HPS[bin-1] *= magFloor(aggregate[i] >> 2);
        HPS[bin] *= magFloor(aggregate[i] >> 2);
        HPS[bin-1] *= magFloor(aggregate[i+1] >> 5);
        HPS[bin] *= magFloor(aggregate[i+1] >> 1);
        HPS[bin] *= magFloor(aggregate[i+2]);
        HPS[bin] *= magFloor(aggregate[i+3] >> 1);
        HPS[bin+1] *= magFloor(aggregate[i+3] >> 5);
        HPS[bin] *= magFloor(aggregate[i+4] >> 2);
        HPS[bin+1] *= magFloor(aggregate[i+4] >> 2);
        bin++;
    }

    HPS[81] *= magFloor(aggregate[284] >> 2);
    HPS[82] *= magFloor(aggregate[284] >> 2);
    HPS[81] *= magFloor(aggregate[285] >> 5);
    HPS[82] *= magFloor(aggregate[285] >> 1);
    HPS[82] *= magFloor(aggregate[286]);
    HPS[82] *= magFloor(aggregate[287] >> 1);

    HPS[12] = magFloor(HPS[12] >> 1);

    for(i = 21; i < 30;i+=4){
        HPS[i] = HPS[i] << 1;
    }

    HPS[31] = magFloor(HPS[31] >> 1);

    /*for(i = 32; i < 47; i+=2){
        HPS[i] = magFloor(HPS[i] >> 1);
    }*/

    for(i = 43; i < 48; i++){
        HPS[i] =  magFloor(HPS[i] >> 1);
    }

    for(i = 31; i < 34; i++){
        HPS[i] =  magFloor(HPS[i] << 1);
    }

    HPS[48] = magFloor(HPS[48] >> 2);

    for(i = 19; i < 24; i++){
        HPS[i] = magFloor(HPS[i] >> 1);
    }

    HPS[26] = magFloor(HPS[26] >> 2);
    HPS[27] = magFloor(HPS[27] >> 2);
}

void detectOutput(void){
    int i, j;

    for(i = 0; i < 288; i++){
            rms += aggregate[i]*aggregate[i];
    }

    rms = rms / (288 * 288 / 50 / 50);
    rms = (uint32_t)_IQsqrt((int32_t)rms << 1) >> 1;

    uint32_t threshold = thresholds[0];

    if(rms >= 15 /*.3 * 50*/){
        threshold = thresholds[(uint32_t) (rms - 15)];
    }

    //
    threshold = 2000;

    char tempOut[6][3] = {{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}};
    int index = 0;
    int jump = 0;
    int highest = 0;
    for(i = 12; i < 86; i++){
        if(HPS[i] > threshold){
            if(HPS[i+1] > threshold){
                    jump++;
                if(HPS[i+1] > HPS[i+highest]){
                    highest = 1;
                }
                if(HPS[i+2] > threshold){
                    jump++;
                    if(HPS[i+2] > HPS[i+highest]){
                        highest = 2;
                    }
                }
            }
            if(index != 6){
                tempOut[index][0] = binToMidi[i+highest];//output here hs i+1
                tempOut[index][1] = 127;
                tempOut[index][2] = i+ highest;
                i+= jump;
                index++;
            }else{
                int lowest = 0;
                for(j = 1; j < 6; j++){
                    if(HPS[tempOut[j][2]] > HPS[tempOut[lowest][2]]){
                        lowest = j;
                    }
                }

                if(HPS[i+highest] > HPS[tempOut[lowest][2]]){
                    tempOut[lowest][0] = binToMidi[i+highest];//output here hs i+1
                    tempOut[lowest][1] = 127;
                    tempOut[lowest][2] = i+ highest;
                    i += jump;
                }
            }
        }
    }

    for(i = 0; i < 6;i++){
        if(!((output[i][2] < 37 && HPS[output[i][2]] > threshold) || (output[i][2] > 36 && HPS[output[i][2]] > threshold))){
            output[i][0] = 0;
            output[i][1] = 0;
            output[i][2] = 0;
        }else{
            for(j = 0; j < 6; j++){
                if(tempOut[j][0] == output[i][0] || tempOut[j][0] == (output[i][0] + 1)|| tempOut[j][0] == (output[i][0] - 1)){
                    tempOut[j][0] = 0;
                    tempOut[j][1] = 0;
                    tempOut[j][2] = 0;
                }
            }
        }
    }

    int isWritten = 0;
    for(i = 0; i < 6; i++){
        if(tempOut[i][0] != 0){
            for(j = 0; j < 6; j++){
                if(output[j][0] == 0){
                    isWritten = 1;
                    output[j][0] = tempOut[i][0];
                    output[j][1] = tempOut[i][1];
                    output[j][2] = tempOut[i][2];
                    break;
                }
            }
            if(isWritten == 0){
                int lowest = 0;
                for(j = 1; j < 6; j++){
                    if(HPS[output[j][2]] > HPS[output[lowest][2]]){
                        lowest = j;
                    }
                }
                output[lowest][0] = tempOut[i][0];
                output[lowest][1] = tempOut[i][1];
                output[lowest][2] = tempOut[i][2];
            }
            isWritten == 0;
        }
    }
}

void setSpectrum(uint8_t *topSpec, uint8_t *midSpec, uint8_t *lowSpec){
    spectrum2 = topSpec;
    spectrum1 = midSpec;
    spectrum0 = lowSpec;
}

void generateOutput(void){
    generateAggregateSpec();
    generateHPS();
    detectOutput();
}

char** getOutput(void){
    return output;
}
