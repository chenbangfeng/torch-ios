#ifndef TH_GENERIC_FILE
#define TH_GENERIC_FILE "generic/VolumetricConvolution.c"
#else

static int nn_(VolumetricConvolution_forward)(lua_State *L)
{
  THTensor *input = luaT_checkudata(L, 2, torch_(Tensor_id));  
  int dT = luaT_getfieldcheckint(L, 1, "dT");
  int dW = luaT_getfieldcheckint(L, 1, "dW");
  int dH = luaT_getfieldcheckint(L, 1, "dH");

  THTensor *weight = luaT_getfieldcheckudata(L, 1, "weight", torch_(Tensor_id));
  THTensor *bias = luaT_getfieldcheckudata(L, 1, "bias", torch_(Tensor_id));
  THTensor *output = luaT_getfieldcheckudata(L, 1, "output", torch_(Tensor_id));

  luaL_argcheck(L, input->nDimension == 4, 2, "4D tensor expected");

  long nOutputPlane = weight->size[0];
  long nInputPlane  = weight->size[1];
  long kT           = weight->size[2];
  long kH           = weight->size[3];
  long kW           = weight->size[4];
  long inputDepth   = input->size[1];
  long inputHeight  = input->size[2];
  long inputWidth   = input->size[3];
  long outputDepth  = (inputDepth - kT) / dT + 1;
  long outputWidth  = (inputWidth - kW) / dW + 1;
  long outputHeight = (inputHeight - kH) / dH + 1;

  THTensor_(resize4d)(output, nOutputPlane, outputDepth, outputHeight, outputWidth);

  /* add bias */
  long i;
  THTensor *outn = THTensor_(new)();
  for (i=0; i<bias->size[0]; i++) {
    THTensor_(select)(outn,output,0,i);
    THTensor_(fill)(outn, THTensor_(get1d)(bias, i));
  }
  THTensor_(free)(outn);

  /* do convolutions */
  THLab_(conv3Dmv)(output, 1.0, 1.0, input, weight, dT, dH, dW, "vx");

  return 1;
}


static int nn_(VolumetricConvolution_backward)(lua_State *L)
{
  THTensor *input = luaT_checkudata(L, 2, torch_(Tensor_id));  
  THTensor *gradOutput = luaT_checkudata(L, 3, torch_(Tensor_id));  
  int dT = luaT_getfieldcheckint(L, 1, "dT");
  int dW = luaT_getfieldcheckint(L, 1, "dW");
  int dH = luaT_getfieldcheckint(L, 1, "dH");
  int nOutputPlane = luaT_getfieldcheckint(L, 1, "nOutputPlane");

  THTensor *weight = luaT_getfieldcheckudata(L, 1, "weight", torch_(Tensor_id));
  THTensor *gradWeight = luaT_getfieldcheckudata(L, 1, "gradWeight", torch_(Tensor_id));
  THTensor *gradBias = luaT_getfieldcheckudata(L, 1, "gradBias", torch_(Tensor_id));
  THTensor *gradInput = luaT_getfieldcheckudata(L, 1, "gradInput", torch_(Tensor_id));
  
  THArgCheck( nOutputPlane == gradOutput->size[0], 1, "Number of output features is not equal to nOutputPlane" );

  long k;

  /* gradient to bias */
  real *gradBias_data = THTensor_(data)(gradBias);
  THTensor* gradOutSlice = THTensor_(new)();
  for(k = 0; k < nOutputPlane; k++)
  {
    THTensor_(select)(gradOutSlice, gradOutput, 0, k);
    gradBias_data[k] += THTensor_(sum)(gradOutSlice);
  }
  THTensor_(free)(gradOutSlice);

  /* gradient to kernels */
  THLab_(conv3DRevger)(gradWeight, 1.0, 1.0, input, gradOutput, dT, dH, dW);

  /* gradient to input */
  THTensor *tweight = THTensor_(newTranspose)(weight,0,1);
  THLab_(conv3Dmv)(gradInput, 0.0, 1.0, gradOutput, tweight, dT, dH, dW, "fc");
  THTensor_(free)(tweight);

  return 1;
}

static const struct luaL_Reg nn_(VolumetricConvolution__) [] = {
  {"VolumetricConvolution_forward", nn_(VolumetricConvolution_forward)},
  {"VolumetricConvolution_backward", nn_(VolumetricConvolution_backward)},
  {NULL, NULL}
};

static void nn_(VolumetricConvolution_init)(lua_State *L)
{
  luaT_pushmetaclass(L, torch_(Tensor_id));
  luaT_registeratname(L, nn_(VolumetricConvolution__), "nn");
  lua_pop(L,1);
}

#endif
