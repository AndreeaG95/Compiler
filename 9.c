struct Pt{
	int x,y;
	};

struct Pt		points[200];

int		count()
{
	int		i,n;
	for(i=n=1;i<10;i=i+1){
		if(points[i].x>=1&&points[i].y>=1)n=n+1;
		}
	return n;
}

void main()
{
	put_i(count());
}
