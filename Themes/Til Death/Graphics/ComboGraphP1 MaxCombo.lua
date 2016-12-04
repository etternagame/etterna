return Def.Quad{
	InitCommand=cmd(setsize,1,12;diffuse,getMainColor('highlight');diffusetopedge,Saturation(Brightness(getMainColor('highlight'),1),0.5));
};