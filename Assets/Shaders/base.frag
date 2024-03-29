#version 460

layout(location = 0) out vec4 FragColor;
 
layout(location = 0) in PerVertexData
{
  vec4 color;
  vec2 uv;
} frag_in;  
 
void main()
{
  FragColor = vec4(frag_in.uv, 1.0, 1.0);
}