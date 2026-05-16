/* 2026-05-15T10:20:45.475215 */
/*
* Copyright (c) 2026 Nordic Semiconductor ASA
* SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
*/
#include "nrf_edgeai_user_model.h"
#include "nrf_edgeai_user_types.h"
#include <nrf_edgeai/nrf_edgeai_platform.h>
#include <nrf_edgeai/rt/private/nrf_edgeai_interfaces.h>
#include <assert.h>

//////////////////////////////////////////////////////////////////////////////
/* Nordic EdgeAI Lab Solution ID and Runtime Version */
#define EDGEAI_LAB_SOLUTION_ID_STR      "93194"
#define EDGEAI_RUNTIME_VERSION_COMBINED 0x00000202

//////////////////////////////////////////////////////////////////////////////
#define INPUT_TYPE                         i16

/** User input features type */
#define INPUT_FEATURE_DATA_TYPE            NRF_EDGEAI_INPUT_I16

/** Number of unique features in the original input sample */
#define INPUT_UNIQ_FEATURES_NUM            6

/** Number of unique features actually used by NN from the original input sample */
#define INPUT_UNIQ_FEATURES_USED_NUM       6

/** Number of input feature samples that should be collected in the input window
 *  feature_sample = 1 * INPUT_UNIQ_FEATURES_NUM
 */
#define INPUT_WINDOW_SIZE                  134

/** Number of input feature samples on that the input window is shifted */
#define INPUT_WINDOW_SHIFT                 89

/** Number of subwindows in input feature window,
* the SUBWINDOW_SIZE = INPUT_WINDOW_SIZE / INPUT_SUBWINDOW_NUM
* if the window size is not divisible by the number of subwindows without a remainder,
* the remainder is added to the last subwindow size */
#define INPUT_SUBWINDOW_NUM                 0

#define INPUT_UNIQUE_SCALES_NUM (sizeof(INPUT_FEATURES_SCALE_MIN) / sizeof(INPUT_FEATURES_SCALE_MIN[0])) 

/** Defines input(also used for LAG) features MIN scaling factor
 */
static const nrf_user_input_t INPUT_FEATURES_SCALE_MIN[] = {
 -12291, -12714, -32767, -7285, -11962, -5632 };

/** Defines input(also used for LAG) features MAX scaling factor
 */
static const nrf_user_input_t INPUT_FEATURES_SCALE_MAX[] = {
 8705, 14394, 32748, 5970, 5681, 6988 };

/** Defines which unique features from the input data will be used/collected,
 *  one bit for one unique feature, starting from LSB
 */
#define INPUT_FEATURES_USAGE_MASK NULL

/** Defines which unique input features is used for LAG features processing,
 *  one bit for one unique feature, starting from LSB
 */
#define INPUT_FEATURES_USED_FOR_LAGS_MASK NULL

//////////////////////////////////////////////////////////////////////////////
#define MODEL_TYPE                 __NRF_EDGEAI_MODEL_NEUTON
#define MODEL_TASK                 0
#define MODEL_OUTPUTS_NUM          8

#define MODEL_USES_AS_INPUT_INPUT_FEATURES 0
#define MODEL_USES_AS_INPUT_DSP_FEATURES 1
#define MODEL_USES_AS_INPUT_MASK ((MODEL_USES_AS_INPUT_INPUT_FEATURES << 0) | (MODEL_USES_AS_INPUT_DSP_FEATURES << 1)) 

#if MODEL_TYPE == __NRF_EDGEAI_MODEL_AXON 
#include <drivers/axon/nrf_axon_nn_infer.h>  
#include <axon/nrf_axon_platform.h> 
#include "nrf_edgeai_user_model_axon.h" 
#define P_MODEL_INSTANCE &model_axon_user_instance_93194
#else  // MODEL_TYPE == __NRF_EDGEAI_MODEL_NEUTON
#define P_MODEL_INSTANCE &model_neuton_user_instance_ 
#endif


#define NN_DECODED_OUTPUT_INIT                 \
.classif = {                                   \
   .predicted_class = 0,                       \
   .num_classes = MODEL_OUTPUTS_NUM,           \
}

//////////////////////////////////////////////////////////////////////////////
#define MODEL_NEURONS_NUM          41
#define MODEL_WEIGHTS_NUM          372
#define MODEL_PARAMS_TYPE          f32
#define MODEL_REORDERING           1

static const nrf_user_weight_t MODEL_WEIGHTS[] = {
 -1.0000000, 0.6749657, -0.8571707, 0.3552536, -0.0261005, -0.4581516,
 0.9246736, -0.7725475, 0.2620011, 0.1640424, -0.2496210, -0.2953833,
 -1.0000000, 0.1122255, 0.9941257, -0.0122783, 0.1334175, 0.7121181,
 0.8138895, 0.2095403, 0.2408224, 0.9903986, 0.5541966, -1.0000000,
 -0.9950503, -1.0000000, -1.0000000, -0.5889482, 0.9997736, 0.8749651,
 0.9999996, -0.3570308, 0.9472346, -0.8305973, 0.3264677, 0.2379203,
 -0.6390914, 0.8220927, 1.0000000, 0.6242770, 0.3042402, -0.5932904,
 -0.8666242, -1.0000000, 0.9369090, 0.0362840, -0.5182297, 0.2696768,
 -0.9073911, -0.0250004, -0.9861919, 0.6421502, 0.6941092, 1.0000000,
 -0.2271129, 0.3106591, -0.6684227, -0.8221289, 0.3450438, 0.1047282,
 -0.8660720, 0.8472885, 0.1991601, 0.7370667, -0.2371556, 0.2120272,
 0.2436212, 0.8342975, 0.0492449, -0.8553266, -0.2118857, 0.2133871,
 -0.5662120, 0.9986486, 0.1022935, 0.1498415, 0.0486091, -0.7295164,
 -0.0742734, 0.5384620, 0.0090942, 0.9051595, -0.8315942, 0.5036086,
 -0.3295219, 0.1789551, -0.2338326, -0.8740465, 1.0000000, -0.1319891,
 0.2244330, 0.5363655, -0.9237325, -0.9908218, -0.3622941, 0.1437621,
 -0.5361098, 0.6135110, 1.0000000, 0.0000000, 1.0000000, -1.0000000,
 -0.5050770, 0.8667544, -0.1315669, -1.0000000, 0.2769117, 0.2877423,
 -0.3744661, 0.6023724, -0.8692232, -0.3089448, -0.5737075, -0.1845594,
 0.4827678, 0.8873069, -0.9375000, 0.7500607, 0.9611831, -1.0000000,
 -0.5575594, 0.7518843, -0.9178129, 0.8715202, 0.1560082, 0.7548947,
 -1.0000000, -0.0255820, -0.2486710, 1.0000000, -1.0000000, -0.7811844,
 -0.7950525, 0.2583385, -0.3765466, 1.0000000, -0.9785236, 0.9654680,
 0.0090778, -0.4686296, -1.0000000, 0.5753709, -0.6316864, 0.6537902,
 -0.5588218, -0.8873023, 0.9987397, 0.3508232, -0.0366567, -0.8007759,
 -0.0230024, 0.1063262, -0.9804654, -0.0967771, 0.4528605, -0.3863995,
 0.9528488, -0.2170503, 0.0864616, -0.1324688, -0.7724575, -0.6423357,
 0.6589156, -0.9244304, 0.3590152, 0.2989992, -0.3325452, -0.1191857,
 -0.2003720, 0.3423894, -0.7360482, -0.3565204, 0.1621434, -0.6139541,
 -0.0831091, -0.1543808, 0.8288450, 0.9064581, 0.0176158, -0.5114284,
 0.6886680, 0.5031252, 0.3766751, 0.3277272, -0.8120793, -0.9537546,
 -0.1274594, -0.2933156, -0.2840306, -0.1621672, -0.0707636, 0.5929132,
 -0.4236441, -0.8078072, 0.5807438, -0.7834544, 0.1716475, -0.8544325,
 -0.7725604, 0.7622452, -0.5867120, 0.2468867, -0.5227262, -0.1108095,
 0.5843948, 0.2176137, -0.7065454, -0.7240906, 0.8750000, 0.4261138,
 -0.6953794, 0.1188118, -0.1287264, 0.9999998, 0.7007932, 0.2340654,
 0.3568948, 0.1738660, -1.0000000, -0.3088813, 0.3388524, 1.0000000,
 0.0279199, -1.0000000, 0.5379455, -0.9023660, -0.1796972, -0.2457483,
 0.1691597, -0.8871418, 0.4487363, -0.5179793, -0.0937380, -0.0178791,
 -0.5080916, -0.3985969, -0.3764939, 0.3790635, -0.7004664, 0.5749439,
 0.4631566, -0.0903478, 0.5445592, -0.8676409, -0.5000000, 0.5154229,
 0.9277344, 0.4926522, 0.3538835, -0.1388588, 0.9675438, 0.9467119,
 -0.4614779, -0.7132024, -0.8297001, 0.5065495, 0.0115839, 0.1364099,
 0.4961950, 0.9374884, -0.4604203, -0.1983213, -0.0515064, -0.1184292,
 -0.1577139, 0.2297284, -0.1071348, -1.0000000, -0.6493984, 0.0307167,
 0.4054097, -0.0898801, -1.0000000, 0.9814358, -0.0951008, -0.2320582,
 0.1074248, -0.4330479, -0.8750000, -0.7351415, 0.2612827, 1.0000000,
 -0.3673384, -0.9999999, -0.7983562, 0.1512993, -0.1888891, 0.8795248,
 -0.0676742, 0.3428854, 0.8585371, -0.8219312, -0.9824270, 1.0000000,
 1.0000000, -1.0000000, -0.9999999, -1.0000000, 1.0000000, -0.9999996,
 -0.3940381, -0.2711668, -0.0114902, -0.6970963, 0.7669496, -0.9826444,
 -0.8750000, -0.7717519, -0.3940454, 0.9029347, 0.0269926, -0.9034059,
 -0.5789132, 0.7701143, 0.5238687, 0.9441652, 0.0828048, 0.9531860,
 0.0196387, -0.8494999, 0.1917400, -0.3648078, -0.5000000, -0.1102962,
 0.0810154, -0.3481887, 1.0000000, -0.2321354, -1.0000000, 0.0375730,
 -0.1811257, 0.1523644, 0.9587015, 0.4934452, 0.8605547, -0.2498841,
 -0.5140679, 1.0000000, 0.9266338, -0.9999999, 1.0000000, -1.0000000,
 0.7500000, -0.9687500, 1.0000000, -1.0000000, -1.0000000, 0.1793722,
 0.6754204, 0.6926670, -0.8474492, -1.0000000, -0.1834123, 0.2343552,
 0.6565488, -0.9854596, -0.1399181, -0.0689822, -0.3287371, 0.5621268,
 0.9940332, -0.8514330, -0.8973976, 0.8281373, 0.5853584, -0.0965367,
 -0.9937316, 0.9985352, -1.0000000, 1.0000000, -1.0000000, -0.3086660 };

static const uint16_t MODEL_NEURONS_LINKS[] = {
 6, 56, 0, 0, 1, 2, 3, 4, 6, 7, 10, 12, 18, 19, 27, 33, 41, 43, 47, 48, 52,
 55, 56, 8, 12, 28, 45, 46, 47, 56, 2, 56, 0, 3, 7, 12, 14, 42, 46, 47, 50,
 56, 1, 15, 20, 21, 25, 26, 36, 38, 46, 52, 56, 5, 56, 1, 4, 3, 7, 16, 17,
 21, 23, 26, 27, 28, 33, 42, 53, 56, 3, 6, 12, 29, 33, 46, 56, 0, 7, 0, 11,
 21, 29, 30, 39, 53, 56, 1, 2, 7, 8, 5, 19, 23, 42, 45, 48, 56, 6, 56, 0,
 11, 56, 10, 0, 1, 6, 7, 9, 12, 18, 19, 21, 33, 52, 56, 4, 56, 4, 14, 56,
 1, 7, 14, 16, 18, 35, 42, 56, 7, 16, 56, 1, 8, 9, 13, 3, 6, 15, 21, 29,
 39, 55, 56, 7, 5, 19, 48, 56, 13, 16, 2, 5, 7, 21, 31, 48, 49, 52, 53, 56,
 7, 8, 17, 24, 37, 53, 56, 1, 8, 16, 0, 16, 18, 47, 48, 56, 13, 0, 1, 9,
 18, 21, 22, 24, 33, 56, 13, 1, 4, 7, 11, 12, 13, 14, 18, 21, 22, 27, 56,
 1, 4, 0, 2, 3, 4, 10, 12, 21, 22, 54, 56, 1, 24, 1, 5, 6, 24, 42, 56, 1,
 13, 14, 24, 1, 3, 5, 6, 19, 53, 56, 8, 9, 18, 20, 3, 5, 9, 11, 24, 26, 33,
 34, 48, 55, 56, 9, 1, 9, 16, 18, 32, 33, 48, 56, 1, 4, 14, 20, 24, 25, 4,
 6, 21, 56, 7, 18, 28, 11, 16, 19, 42, 46, 48, 51, 52, 53, 56, 7, 16, 20,
 22, 24, 31, 3, 6, 19, 48, 52, 56, 8, 10, 19, 20, 21, 22, 28, 29, 31, 32,
 56, 1, 4, 13, 18, 24, 30, 31, 19, 21, 53, 55, 56, 7, 8, 9, 13, 20, 1, 10,
 12, 19, 39, 44, 53, 54, 55, 56, 20, 7, 10, 39, 40, 43, 54, 56, 1, 13, 23,
 24, 25, 26, 27, 30, 34, 35, 36, 56, 8, 9, 25, 31, 0, 15, 21, 29, 39, 53,
 56, 1, 7, 9, 16, 25, 0, 11, 56, 9, 18, 38, 39, 56 };

static const uint16_t MODEL_NEURON_INTERNAL_LINKS_NUM[] = {
 0, 3, 23, 31, 32, 42, 54, 57, 70, 79, 91, 98, 102, 104, 117, 120, 123,
 131, 136, 145, 151, 163, 171, 178, 188, 202, 214, 224, 235, 247, 261, 268,
 284, 300, 308, 318, 329, 347, 352, 364, 371 };

static const uint16_t MODEL_NEURON_EXTERNAL_LINKS_NUM[] = {
 2, 23, 30, 32, 42, 53, 55, 70, 77, 87, 98, 100, 103, 116, 118, 121, 129,
 132, 144, 149, 161, 168, 177, 187, 200, 212, 220, 231, 246, 255, 265, 278,
 290, 301, 313, 328, 336, 348, 359, 367, 372 };

static const nrf_user_coeff_t MODEL_NEURON_ACTIVATION_WEIGHTS[] = {
 40.0000000, 40.0000000, 40.0000000, 40.0000000, 40.0000000, 40.0000000,
 40.0000000, 40.0000000, 40.0000000, 40.0000000, 30.0248375, 40.0000000,
 40.0000000, 30.5835800, 39.9999733, 39.9993896, 40.0000000, 40.0000000,
 39.9999771, 40.0000000, 40.0000000, 40.0000000, 40.0000000, 35.0427704,
 35.0427704, 40.0000000, 40.0000000, 40.0000000, 40.0000000, 40.0000000,
 40.0000000, 40.0000000, 40.0000000, 40.0000000, 40.0000000, 40.0000000,
 40.0000000, 40.0000000, 39.9999771, 39.9999771, 39.9999771 };

static const uint8_t MODEL_NEURON_ACTIVATION_TYPE_MASK[] = {
 0xb7, 0x6f, 0xfd, 0xff, 0xdd, 0x0 };

static const uint16_t MODEL_OUTPUT_NEURONS_INDICES[] = {
 12, 37, 3, 15, 6, 17, 33, 40 };

/** Model neurons activations buffer */ 
static nrf_user_neuron_t model_neurons_[MODEL_NEURONS_NUM];

/** Neuton model instance */ 
static const nrf_edgeai_model_neuton_t model_neuton_user_instance_ = { 
   .meta.p_neuron_internal_links_num = MODEL_NEURON_INTERNAL_LINKS_NUM,
   .meta.p_neuron_external_links_num = MODEL_NEURON_EXTERNAL_LINKS_NUM,
   .meta.p_output_neurons_indices    = MODEL_OUTPUT_NEURONS_INDICES,
   .meta.p_neuron_links              = MODEL_NEURONS_LINKS,
   .meta.p_neuron_act_type_mask      = MODEL_NEURON_ACTIVATION_TYPE_MASK,
   .meta.outputs_num                 = MODEL_OUTPUTS_NUM,
   .meta.neurons_num                 = MODEL_NEURONS_NUM,
   .meta.weights_num                 = MODEL_WEIGHTS_NUM,
   /// 
   .params.MODEL_PARAMS_TYPE = {
       .p_weights      = MODEL_WEIGHTS,
       .p_act_weights  = MODEL_NEURON_ACTIVATION_WEIGHTS,
       .p_neurons      = model_neurons_,
   },
};

//////////////////////////////////////////////////////////////////////////////
/** Input feature buffer element size, 
 * if quantization of model is bigger than input features size in bits, 
 * the size of input buffer should aligned to nrf_user_neuron_t */ 
#define INPUT_TYPE_SIZE \
    ((sizeof(nrf_user_input_t) > sizeof(nrf_user_neuron_t)) ? sizeof(nrf_user_input_t) : sizeof(nrf_user_neuron_t)) 

/** Input features window size in bytes to allocate statically */ 
#define INPUT_WINDOW_BUFFER_SIZE_BYTES \
    (INPUT_WINDOW_SIZE * INPUT_UNIQ_FEATURES_NUM * INPUT_TYPE_SIZE) 

static uint8_t input_window_[INPUT_WINDOW_BUFFER_SIZE_BYTES] __NRF_EDGEAI_ALIGNED; 

#define INPUT_WINDOW_MEMORY    &input_window_[0] 

static nrf_edgeai_window_ctx_t input_window_ctx_; 
#define P_INPUT_WINDOW_CTX     &input_window_ctx_ 

//////////////////////////////////////////////////////////////////////////////
/** The maximum number of extracted features that user used for all unique input features */
#define EXTRACTED_FEATURES_NUM  56

#define EXTRACTED_FEATURES_META_TYPE i32 

/** DSP feature buffer element size,
 * if quantization of model is bigger than DSP features size in bits,
 * the size of extracted DSP features buffer should aligned to nrf_user_neuron_t */
#define EXTRACTED_FEATURE_SIZE_BYTES                                                  \
    ((sizeof(nrf_user_feature_t) > sizeof(nrf_user_neuron_t)) ? sizeof(nrf_user_feature_t) : \
                                                            sizeof(nrf_user_neuron_t))

/** Size of extracted features buffer in bytes */
#define EXTRACTED_FEATURES_BUFFER_SIZE_BYTES (EXTRACTED_FEATURES_NUM * EXTRACTED_FEATURE_SIZE_BYTES) 

/** Defines feature extraction masks used as nrf_edgeai_features_mask_t,
 *  64 bit for one unique input feature, @ref nrf_edgeai_features_mask_t to see bitmask
 */

static const uint64_t FEATURES_EXTRACTION_MASK[] = {
 0x879f00000000, 0x469b00000000, 0xc39f00000000, 0xc78c00000000,
 0xc70f00000000, 0xc79f00000000 };

/** Defines arguments used while feature extraction
 */

/** Defines arguments used while feature extraction
 */
#define FEATURES_EXTRACTION_ARGUMENTS NULL

/** Defines extracted features MIN scaling factor
 */
static const nrf_user_feature_t EXTRACTED_FEATURES_SCALE_MIN[] = {
 -12291, -6321, 67, -8373, 10, 15, 100, 7, 0, 13, -12714, -4753, -5919, 9,
 15, 7, 0, 30, -32767, 6210, 63, -1643, 10, 20, 2781, 7, 2563, 13, 4,
 -1853, 0, 4, 7, 0, 3, 0, -11962, -17, 6, -1524, 12, 7, 0, 9, 0, -5632, 12,
 4, -1414, 0, 1, 7, 7, 0, 6, 0 };

/** Defines extracted features MAX scaling factor
 */
static const nrf_user_feature_t EXTRACTED_FEATURES_SCALE_MAX[] = {
 -133, 8705, 16464, -98, 3714, 4113, 8462, 541, 105, 742, 4392, 14394,
 7214, 4510, 4945, 526, 285, 7214, 9976, 32748, 65407, 10008, 9662, 12136,
 13229, 609, 10738, 4771, 11610, 1710, 3269, 3549, 578, 255, 3063, 325,
 -21, 5681, 15722, 1438, 4444, 428, 263, 3313, 653, 31, 6988, 9805, 1175,
 2635, 2949, 3174, 368, 172, 2737, 228 };

/** Memory allocation to store extracted features during DSP pipeline */
static uint8_t extracted_features_buffer_[EXTRACTED_FEATURES_BUFFER_SIZE_BYTES] __NRF_EDGEAI_ALIGNED;


/** Timedomain features processing context  */
#define P_TIMEDOMAIN_FEATURES_CTX  NULL
/** Timedomain features in feature extraction pipeline  */
static const nrf_edgeai_features_pipeline_func_i16_t timedomain_features_[] = {
    nrf_edgeai_feature_utility_tss_sum_i16,
    nrf_edgeai_feature_min_max_range_i16,
    nrf_edgeai_feature_mean_i16,
    nrf_edgeai_feature_mad_i16,
    nrf_edgeai_feature_std_i16,
    nrf_edgeai_feature_rms_i16,
    nrf_edgeai_feature_mcr_i16,
    nrf_edgeai_feature_zcr_i16,
    nrf_edgeai_feature_absmean_i16,
    nrf_edgeai_feature_amdf_i16
 };

static const nrf_edgeai_features_pipeline_ctx_t timedomain_pipeline_ = {
    .functions_num     = sizeof(timedomain_features_) / sizeof(timedomain_features_[0]),
    .functions.p_void  = timedomain_features_,
    .p_ctx             = P_TIMEDOMAIN_FEATURES_CTX,
};
#define P_TIMEDOMAIN_PIPELINE &timedomain_pipeline_ 

#define P_FREQDOMAIN_PIPELINE NULL

#define P_CUSTOMDOMAIN_PIPELINE NULL

static nrf_edgeai_dsp_pipeline_t dsp_pipeline_ = { 
   .features = {  
       .p_masks = (const nrf_edgeai_features_mask_t*)FEATURES_EXTRACTION_MASK, 
       .buffer.p_void = extracted_features_buffer_, 
       .overall_num = EXTRACTED_FEATURES_NUM, 
       .masks_num = sizeof(FEATURES_EXTRACTION_MASK) / sizeof(FEATURES_EXTRACTION_MASK[0]), 

       .p_timedomain_pipeline = P_TIMEDOMAIN_PIPELINE, 
       .p_freqdomain_pipeline = P_FREQDOMAIN_PIPELINE, 
       .p_customdomain_pipeline = P_CUSTOMDOMAIN_PIPELINE, 

       .meta.EXTRACTED_FEATURES_META_TYPE = { 
           .p_min = EXTRACTED_FEATURES_SCALE_MIN, 
           .p_max = EXTRACTED_FEATURES_SCALE_MAX, 
       .p_arguments = FEATURES_EXTRACTION_ARGUMENTS, 
       },
   }, 
}; 

#define P_DSP_PIPELINE         &dsp_pipeline_ 


//////////////////////////////////////////////////////////////////////////////
#define NN_INPUT_INIT_INTERFACE        nrf_edgeai_input_init_sliding_window 
#define NN_INPUT_FEED_INTERFACE        nrf_edgeai_input_feed_sliding_window_i16 
#define NN_PROCESS_FEATURES_INTERFACE  nrf_edgeai_process_features_dsp_i16_f32 
#define NN_INIT_INFERENCE_INTERFACE    nrf_edgeai_init_inference_neuton 
#define NN_RUN_INFERENCE_INTERFACE     nrf_edgeai_run_inference_neuton_f32 
#define NN_PROPAGATE_OUTPUTS_INTERFACE nrf_edgeai_output_propagate_neuton_f32 
#define NN_DECODE_OUTPUTS_INTERFACE    nrf_edgeai_output_decode_classification_f32 

//////////////////////////////////////////////////////////////////////////////

static nrf_user_output_t model_outputs_[MODEL_OUTPUTS_NUM];

//////////////////////////////////////////////////////////////////////////////

static nrf_edgeai_t nrf_edgeai_ = {
    ///
    .metadata.p_solution_id     = EDGEAI_LAB_SOLUTION_ID_STR,
    .metadata.version.combined  = EDGEAI_RUNTIME_VERSION_COMBINED,
    ///   
    .input.p_used_for_lags_mask = INPUT_FEATURES_USED_FOR_LAGS_MASK,
    .input.p_usage_mask         = INPUT_FEATURES_USAGE_MASK,
    .input.type                 = INPUT_FEATURE_DATA_TYPE,
    .input.unique_num           = INPUT_UNIQ_FEATURES_NUM,
    .input.unique_num_used      = INPUT_UNIQ_FEATURES_USED_NUM,
    .input.unique_scales_num    = INPUT_UNIQUE_SCALES_NUM,
    .input.window_size          = INPUT_WINDOW_SIZE,
    .input.window_shift         = INPUT_WINDOW_SHIFT,
    .input.subwindow_num        = INPUT_SUBWINDOW_NUM,
    .input.window_memory.p_void = INPUT_WINDOW_MEMORY,
    .input.p_window_ctx         = P_INPUT_WINDOW_CTX,

    .input.scale.INPUT_TYPE = {
        .p_min = INPUT_FEATURES_SCALE_MIN,
        .p_max = INPUT_FEATURES_SCALE_MAX,
    }, 
    ///
    .p_dsp = P_DSP_PIPELINE,
    ///
    .model.type                 = (nrf_edgeai_model_type_t)MODEL_TYPE,
    .model.task                 = (nrf_edgeai_model_task_t)MODEL_TASK,
    .model.instance.p_void      = P_MODEL_INSTANCE,
    .model.output.memory.p_void = model_outputs_,
    .model.output.num           = MODEL_OUTPUTS_NUM,
    .model.uses_as_input.all    = MODEL_USES_AS_INPUT_MASK,
    ///
    .interfaces.input_init          = NN_INPUT_INIT_INTERFACE,
    .interfaces.feed_inputs         = NN_INPUT_FEED_INTERFACE,
    .interfaces.process_features    = NN_PROCESS_FEATURES_INTERFACE,
    .interfaces.init_inference      = NN_INIT_INFERENCE_INTERFACE,
    .interfaces.run_inference       = NN_RUN_INFERENCE_INTERFACE,
    .interfaces.propagate_outputs   = NN_PROPAGATE_OUTPUTS_INTERFACE,
    .interfaces.decode_outputs      = NN_DECODE_OUTPUTS_INTERFACE,
    ///
    .decoded_output = { NN_DECODED_OUTPUT_INIT },
};

//////////////////////////////////////////////////////////////////////////////

nrf_edgeai_t* nrf_edgeai_user_model_93194(void)
{
    return &nrf_edgeai_;
}

//////////////////////////////////////////////////////////////////////////////

uint32_t nrf_edgeai_user_model_neuton_size_93194(void)
{
    uint32_t model_meta_size = 0;
#if MODEL_TYPE == __NRF_EDGEAI_MODEL_NEUTON
    model_meta_size +=
        (sizeof(MODEL_WEIGHTS) + sizeof(MODEL_NEURONS_LINKS) +
         sizeof(MODEL_NEURON_EXTERNAL_LINKS_NUM) + sizeof(MODEL_NEURON_INTERNAL_LINKS_NUM) +
         sizeof(MODEL_NEURON_ACTIVATION_WEIGHTS) + sizeof(MODEL_NEURON_ACTIVATION_TYPE_MASK) +
         sizeof(MODEL_OUTPUT_NEURONS_INDICES));
#endif

#if MODEL_TASK == __NRF_EDGEAI_TASK_ANOMALY_DETECTION
    model_meta_size += sizeof(MODEL_AVERAGE_EMBEDDING) + sizeof(MODEL_OUTPUT_SCALE_MIN) +
                       sizeof(MODEL_OUTPUT_SCALE_MAX);
#endif

#if MODEL_TASK == __NRF_EDGEAI_TASK_REGRESSION
    model_meta_size += sizeof(MODEL_OUTPUT_SCALE_MIN) + sizeof(MODEL_OUTPUT_SCALE_MAX);
#endif

    return model_meta_size;
}


