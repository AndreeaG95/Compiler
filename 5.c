void main()
{
	int		i,n;
	double	su;
	su=3.2;
	put_s("n=");
	n=get_i();
	for(i=1;i<n;i=i+1){
		su=su+get_i();
		}
	put_s("media=");
	put_d(su/n);
}
