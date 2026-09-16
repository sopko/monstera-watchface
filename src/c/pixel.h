#pragma once
// Original 5 by 7 bitmap alphabet; columns encoded top-to-bottom.
static const uint8_t digits[10][5] = {
 {62,65,65,65,62},{0,66,127,64,0},{98,81,73,73,70},{34,65,73,73,54},
 {24,20,18,127,16},{39,69,69,69,57},{62,73,73,73,50},{1,113,9,5,3},
 {54,73,73,73,54},{38,73,73,73,62}};
static const uint8_t letters[26][5] = {
 {126,9,9,9,126},{127,73,73,73,54},{62,65,65,65,34},{127,65,65,34,28},
 {127,73,73,73,65},{127,9,9,9,1},{62,65,73,73,122},{127,8,8,8,127},
 {0,65,127,65,0},{32,64,65,63,1},{127,8,20,34,65},{127,64,64,64,64},
 {127,2,12,2,127},{127,4,8,16,127},{62,65,65,65,62},{127,9,9,9,6},
 {62,65,81,33,94},{127,9,25,41,70},{38,73,73,73,50},{1,1,127,1,1},
 {63,64,64,64,63},{31,32,64,32,31},{63,64,56,64,63},{99,20,8,20,99},
 {3,4,120,4,3},{97,81,73,69,67}};
static void pixel_text(GContext *ctx, const char *s, int x, int y, int sx, int sy) {
 graphics_context_set_fill_color(ctx,GColorBlack);
 for (; *s; s++) {
  uint8_t other[5]={0}; const uint8_t *g=other;
  if (*s>='0' && *s<='9') g=digits[*s-'0'];
  else if (*s>='A' && *s<='Z') g=letters[*s-'A'];
  else if (*s==':') {other[1]=54;other[2]=54;}
  else if (*s=='-') {other[0]=8;other[1]=8;other[2]=8;other[3]=8;}
  else if (*s=='+') {other[0]=8;other[1]=8;other[2]=62;other[3]=8;other[4]=8;}
  else if (*s=='%') {other[0]=99;other[1]=19;other[2]=8;other[3]=100;other[4]=99;}
  else if (*s==',') {other[1]=96;other[2]=32;}
  else if (*s=='~') {other[0]=6;other[1]=9;other[2]=9;other[3]=6;}
  int width=*s==':'?3:5;
  for(int c=0;c<width;c++) for(int r=0;r<7;r++)
   if(g[c]&(1<<r)) graphics_fill_rect(ctx,GRect(x+c*sx,y+r*sy,sx+(sx==3?1:0),sy),0,GCornerNone);
  x+=(width+1)*sx;
 }
}
