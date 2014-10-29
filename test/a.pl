$a = "eeee/xxx.jpg";

if ($a=~/(.*)\/(.*)\.(jpg|png|html)$/)
{
   print "$1\n";
}
