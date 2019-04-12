// Definitions used for compression
//

#define CODE_TREE1_RLE   3 // A RLE code, following # of repetitions

#define CODE_NHW_NB_1    1008
#define CODE_NHW_NB_2    1009
#define CODE_NHW_NB_3    1010
#define CODE_NHW_NB_4    1011
#define CODE_NHW_NB_5    1006
#define CODE_NHW_NB_6    1007

#define IS_NHW_CODE(x) ((x) > 1000)
#define CODE_WORD_120   120
#define CODE_WORD_121   121
#define CODE_WORD_122   122
#define CODE_WORD_123   123
#define CODE_WORD_124   124
#define CODE_WORD_125   125
#define CODE_WORD_126   126
#define CODE_WORD_127   127
#define CODE_WORD_128   128 // Special occurence
#define CODE_WORD_129   129

#define CODE_WORD_130   130
#define CODE_WORD_132   132
#define CODE_WORD_133   133
#define CODE_WORD_134   134
#define CODE_WORD_135   135
#define CODE_WORD_136   136


#define IS_CODE_WORD(x)    ((x) <= 136 && (x) >= 120)
#define IS_CODE_WORD_1(x)  ((x) < 136 && (x) > 120)
