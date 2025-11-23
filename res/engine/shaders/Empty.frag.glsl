// An empty fragment shader needed for depth only (vertex shader only) passes.
// TLDR: on some Intel and AMD GPUs this saves us from a GL error in some cases.
// For example: for shadow pass we only need to draw depth and if we use the original shader program
// (with both vertex and fragment shaders), disable color writes and bind only vertex shader uniforms
// glDrawElements will give us INVALID_OPERATION on some Intel and AMD GPUs instead we use a separate
// shader program which only has vertex shader and this empty fragment shader and everything then works fine.

layout(early_fragment_tests) in;
void main(){
}
