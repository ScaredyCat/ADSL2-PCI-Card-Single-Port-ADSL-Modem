/*****************************************************************
Tooltips copyright by l.toetsch@magnet.at, http://www.vvg.or.at
if you use this script keep this note
*****************************************************************/

var a,tipEl,tim1=null,left,top;
function moveTo(xPos,yPos) {
this.style.pixelLeft=xPos;
this.style.pixelTop=yPos;
}
function showIt(on) {
if (NS4) {this.visibility = (on) ? "show" : "hide"}
else {this.style.visibility = (on) ? "visible" : "hidden"}
}
function getDocClientWidth() {
if (NS4)
return window.innerWidth;
else
return document.body.clientWidth;
}
function gHeight() {
if (NS4)
return this.document.height;
else
return this.offsetHeight;
}
function gWidth() {
if (NS4)
return this.document.width;
else
return this.offsetWidth;
}
function init_tt() {
for (i=0;i<document.links.length;i++) {
if(gettext(i)=='')
continue;	
document.links[i].onmouseover=show;
document.links[i].onmouseout=hide;
document.links[i].num=i;
}
tipEl=IE4?eval(document.all['tip']):eval(document.layers.tip);
if(IE4) tipEl.moveTo=moveTo;
tipEl.showIt=showIt;
tipEl.getHeight=gHeight;
tipEl.getWidth=gWidth;
}
function show(e) {
a=this.num;
if (tim1) clearTimeout(tim1);
left = ((NS4) ? e.pageX : event.clientX+document.body.scrollLeft)+3;
top = ((NS4) ? e.pageY : event.clientY+document.body.scrollTop)-3;
tim1 = setTimeout("disp()", 500);
}
function hide() {
tipEl.showIt(false);
a = null;
if (tim1) clearTimeout(tim1);
}
function max(a,b){return a>b?a:b;}
function min(a,b){return a<b?a:b;}
function gettext(a) {
var i;
//a+' '+document.links[a].href;
for(i=0;i<refs.length;i++)
if(document.links[a].href.indexOf(refs[i])>0) 
 return txts[i];
return '';
}

function td(pic,w,h)
{ return '<TD><IMG SRC="http://lux.leo.home/telrate/r'+pic+'.gif" WIDTH="'+w+'" HEIGHT="'+h+'"></TD>';}
function disp() {
txt=gettext(a);
if(txt=='')
return;	
txt+='<br><p>Klicken Sie f&uuml;r weitere Information!';
show1(0);
if(IE4)
setTimeout("show1(1)",1);
else
show1(1);
}
function show1(i) {
h=0;w=250;e=30;
tab='<TABLE CELLPADDING="0" CELLSPACING="0" border="0"><TR>';
if(i){
h=tipEl.getHeight();
tab+=td("liob",e,e);
tab+=td("ob",w,e)+td("reob",e,e)+'</TR>';
tab+='<TR>'+td("li",e,h);
}
tab+='<TD width="'+w+'" BGCOLOR="#f5f5dc" STYLE="'+sty+'">'+txt+'</TD>';
if(i){
x=getDocClientWidth()-2*e-w-10;y=top-h-2*e;
tab+=td("re",e,h)+'</TR><TR>'+td("liun",e,e);
tab+=td("un",w,e)+td("reun",e,e);
}
tab+='</TR></TABLE>';
if(IE4)
tipEl.innerHTML=tab;
else with(tipEl.document) { open();write(tab);close();}
if(i) {
o=IE4?document.body.scrollTop:window.pageYOffset;
tipEl.moveTo(min(x,left),max(10+o,y));
tipEl.showIt(true);
}
}
if (NS4) {      
	origWidth = innerWidth;      
	origHeight = innerHeight;   
}
function reDo() {   
	if (innerWidth != origWidth || innerHeight != origHeight) 
//		init_tt();
      location.reload();
}
if (NS4) window.onresize = reDo;  
