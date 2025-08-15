// #include "keymap_initializer.h"
// #include <stdlib.h>
// #include <string.h>
// #include "pipeline_key_replacer.h"
// #include "pipeline_key_replacer_initializer.h"
// #include "pipeline_oneshot_modifier.h"
// #include "pipeline_oneshot_modifier_initializer.h"
// #include "pipeline_tap_dance.h"
// #include "pipeline_tap_dance_initializer.h"
// #include "platform_interface.h"

// // Begin Key Replacer

// void callback_ckc_replace_shifted_2(replacer_callback_input* input, replacer_callback_result* result) {
//     result->keycode = KC_2;
//     result->modifiers = MACRO_KEY_MODIFIER_LEFT_SHIFT;
// }

// void callback_ckc_replace_shifted_3(replacer_callback_input* input, replacer_callback_result* result) {
//     result->keycode = KC_3;
//     result->modifiers = MACRO_KEY_MODIFIER_LEFT_SHIFT;
// }

// void callback_ckc_replace_shifted_4(replacer_callback_input* input, replacer_callback_result* result) {
//     result->keycode = KC_4;
//     result->modifiers = MACRO_KEY_MODIFIER_LEFT_SHIFT;
// }

// void callback_ckc_replace_shifted_5(replacer_callback_input* input, replacer_callback_result* result) {
//     result->keycode = KC_5;
//     result->modifiers = MACRO_KEY_MODIFIER_LEFT_SHIFT;
// }

// void callback_ckc_replace_shifted_6(replacer_callback_input* input, replacer_callback_result* result) {
//     result->keycode = KC_6;
//     result->modifiers = MACRO_KEY_MODIFIER_LEFT_SHIFT;
// }

// void callback_ckc_replace_shifted_9(replacer_callback_input* input, replacer_callback_result* result) {
//     result->keycode = KC_9;
//     result->modifiers = MACRO_KEY_MODIFIER_LEFT_SHIFT;
// }

// void callback_ckc_replace_shifted_0(replacer_callback_input* input, replacer_callback_result* result) {
//     result->keycode = KC_0;
//     result->modifiers = MACRO_KEY_MODIFIER_LEFT_SHIFT;
// }

// pipeline_key_replacer_global_config_t* pipeline_key_replacer_global_config_create(void) {
//     size_t n_elements = 7;

//     pipeline_key_replacer_global_config_t* global_config = malloc(sizeof(pipeline_key_replacer_global_config_t));
//     if (!global_config) {
//         return NULL;
//     }
//
//     global_config->length = n_elements;
//     global_config->modifier_pairs = malloc(sizeof(pipeline_key_replacer_pair_t*) * n_elements);
//     if (!global_config->modifier_pairs) {
//         free(global_config);
//         return NULL;
//     }
//     global_config->modifier_pairs[0] = pipeline_key_replacer_create_pairs(CKC_REPLACE_SHIFTED_2, callback_ckc_replace_shifted_2);
//     global_config->modifier_pairs[1] = pipeline_key_replacer_create_pairs(CKC_REPLACE_SHIFTED_3, callback_ckc_replace_shifted_3);
//     global_config->modifier_pairs[2] = pipeline_key_replacer_create_pairs(CKC_REPLACE_SHIFTED_4, callback_ckc_replace_shifted_4);
//     global_config->modifier_pairs[3] = pipeline_key_replacer_create_pairs(CKC_REPLACE_SHIFTED_5, callback_ckc_replace_shifted_5);
//     global_config->modifier_pairs[4] = pipeline_key_replacer_create_pairs(CKC_REPLACE_SHIFTED_6, callback_ckc_replace_shifted_6);
//     global_config->modifier_pairs[5] = pipeline_key_replacer_create_pairs(CKC_REPLACE_SHIFTED_9, callback_ckc_replace_shifted_9);
//     global_config->modifier_pairs[6] = pipeline_key_replacer_create_pairs(CKC_REPLACE_SHIFTED_0, callback_ckc_replace_shifted_0);
//     return global_config;
// }

// // End Key Replacer

// // Begin Oneshot Modifier

// pipeline_oneshot_modifier_global_config_t* pipeline_oneshot_modifier_global_config_create(void) {
//     size_t n_elements = 8;

//     pipeline_oneshot_modifier_global_config_t* global_config = malloc(sizeof(pipeline_oneshot_modifier_global_config_t));
//     if (!global_config) {
//         return NULL;
//     }
//
//     global_config->length = n_elements;
//     global_config->modifier_pairs = malloc(sizeof(pipeline_oneshot_modifier_pair_t*) * n_elements);
//     if (!global_config->modifier_pairs) {
//         free(global_config);
//         return NULL;
//     }
//     global_config->modifier_pairs[0] = pipeline_oneshot_modifier_create_pairs(CKC_ONESHOT_MODIFIER_LEFT_CTRL, MACRO_KEY_MODIFIER_LEFT_CTRL);
//     global_config->modifier_pairs[1] = pipeline_oneshot_modifier_create_pairs(CKC_ONESHOT_MODIFIER_LEFT_ALT, MACRO_KEY_MODIFIER_LEFT_ALT);
//     global_config->modifier_pairs[2] = pipeline_oneshot_modifier_create_pairs(CKC_ONESHOT_MODIFIER_LEFT_SHIFT, MACRO_KEY_MODIFIER_LEFT_SHIFT);
//     global_config->modifier_pairs[3] = pipeline_oneshot_modifier_create_pairs(CKC_ONESHOT_MODIFIER_LEFT_GUI, MACRO_KEY_MODIFIER_LEFT_GUI);
//     global_config->modifier_pairs[4] = pipeline_oneshot_modifier_create_pairs(CKC_ONESHOT_MODIFIER_RIGHT_CTRL, MACRO_KEY_MODIFIER_RIGHT_CTRL);
//     global_config->modifier_pairs[5] = pipeline_oneshot_modifier_create_pairs(CKC_ONESHOT_MODIFIER_RIGHT_ALT, MACRO_KEY_MODIFIER_RIGHT_ALT);
//     global_config->modifier_pairs[6] = pipeline_oneshot_modifier_create_pairs(CKC_ONESHOT_MODIFIER_RIGHT_SHIFT, MACRO_KEY_MODIFIER_RIGHT_SHIFT);
//     global_config->modifier_pairs[7] = pipeline_oneshot_modifier_create_pairs(CKC_ONESHOT_MODIFIER_RIGHT_GUI, MACRO_KEY_MODIFIER_RIGHT_GUI);
//     return global_config;
// }

// // End Oneshot Modifier

// // Begin Tap Dance

// pipeline_tap_dance_global_config_t* pipeline_tap_dance_global_config_create(void) {
//     size_t n_elements = 6;

//     pipeline_tap_dance_global_config_t* global_config = malloc(sizeof *global_config);
//     global_config->length = n_elements;
//     global_config->behaviours = malloc(n_elements * sizeof(pipeline_tap_dance_behaviour_t*));

//     pipeline_tap_dance_action_config_t* custom_actions[] =
//          {
//              createbehaviouraction(0, TDCL_TAP_KEY_SENDKEY, KC_Q, _LMOUSE),
//              createbehaviouraction(0, TDCL_HOLD_KEY_CHANGELAYERTEMPO, KC_Q, _LMOUSE)
//          };
//     global_config->behaviours[0] = createbehaviour(CKC_LAY_MOUSE_Q, custom_actions, 2);

//     pipeline_tap_dance_action_config_t* custom_actions2[] =
//          {
//              createbehaviouraction(0, TDCL_TAP_KEY_SENDKEY, KC_R, _LNUMBERS),
//              createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, S(KC_R), _LNUMBERS),
//              createbehaviouraction(0, TDCL_HOLD_KEY_CHANGELAYERTEMPO, KC_R, _LNUMBERS)
//          };
//     global_config->behaviours[1] = createbehaviour(CKC_LAY_NUMBERS_R, custom_actions2, 3);

//     pipeline_tap_dance_action_config_t* custom_actions3[] =
//          {
//              createbehaviouraction(0, TDCL_TAP_KEY_SENDKEY, KC_F, _LNUMBERS),
//              createbehaviouraction(0, TDCL_HOLD_KEY_CHANGELAYERTEMPO, KC_F, _LMOVEMENT),
//              createbehaviouraction(1, TDCL_HOLD_KEY_CHANGELAYERTEMPO, KC_F, _LMOUSE)
//          };
//     global_config->behaviours[2] = createbehaviour(CKC_LAY_MOVEMENT_F, custom_actions3, 3);

//     pipeline_tap_dance_action_config_t* custom_actions4[] =
//          {
//              createbehaviouraction(0, TDCL_TAP_KEY_SENDKEY, KC_EXLM, _LMOUSE),
//              createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, RALT(KC_1), _LNUMBERS),
//          };
//     global_config->behaviours[3] = createbehaviour(CKC_EXCL, custom_actions4, 2);

//     pipeline_tap_dance_action_config_t* custom_actions5[] =
//          {
//              createbehaviouraction(0, TDCL_TAP_KEY_SENDKEY, KC_QUES, _LNUMBERS),
//              createbehaviouraction(1, TDCL_TAP_KEY_SENDKEY, RALT(KC_SLSH), _LNUMBERS),
//          };
//     global_config->behaviours[4] = createbehaviour(CKC_QUES, custom_actions5, 2);

//     pipeline_tap_dance_action_config_t* custom_actions6[] =
//          {
//              createbehaviouraction(0, TDCL_HOLD_KEY_CHANGELAYERTEMPO, KC_QUES, _LRIGHT_THUMB),
//          };
//     global_config->behaviours[5] = createbehaviour(CKC_LAY_RIGHT_THUMB, custom_actions6, 1);

//     return global_config;
// }

// // End Tap Dance
