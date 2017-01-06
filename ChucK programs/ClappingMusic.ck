//Steve Reich - Clapping music 
//interrpretted in ChucK by Liam Caveney


(6.0/(18.4*2))::second => dur b;


SndBuf clapL => dac.left;
0.5 => clapL.gain;
me.dir()+"clap.wav" => clapL.read;

SndBuf clapR => dac.right;
0.5 => clapR.gain;
me.dir()+"clap.wav" => clapR.read;

fun void patternReg(){
	while(true){
		0=>clapL.pos;
		b=>now;
		0=>clapL.pos;
		b=>now;
		0=>clapL.pos;
		b=>now;
		b=>now;
		0=>clapL.pos;
		b=>now;
		0=>clapL.pos;
		b=>now;
		b=>now;
		0=>clapL.pos;
		b=>now;
		b=>now;
		0=>clapL.pos;
		b=>now;
		0=>clapL.pos;
		b=>now;
		b=>now;
	}
}

fun void patternShift(){
	0=>int count;
	while(true){
		for(0=>int i; i<12; i++){
			if(i!=0 || count%12==0){
				0=>clapR.pos;
			}
			b=>now;
			0=>clapR.pos;
			b=>now;
			0=>clapR.pos;
			b=>now;
			b=>now;
			0=>clapR.pos;
			b=>now;
			0=>clapR.pos;
			b=>now;
			b=>now;
			0=>clapR.pos;
			b=>now;
			b=>now;
			0=>clapR.pos;
			b=>now;
			0=>clapR.pos;
			b=>now;
			if(i!=11){
				b=>now;
			}	
		}
		count++;
	}
}


spork ~ patternReg();
spork ~ patternShift();

while( true ) 1::second => now;