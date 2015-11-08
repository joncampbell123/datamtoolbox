function ss(w,id)
{
window.status = w;
return true;
}
function cs()
{
window.status='';
}
function ca(a)
{
window.open(document.getElementById(a).href);
}
function ga(o,e)
{
if (document.getElementById)
{
        a=o.id.substring(1);
        p = "";
        r = "";
        g = e.target;
        if (g)
        {
                t = g.id;f = g.parentNode;
                if (f)
                {
                p = f.id;h = f.parentNode;if (h)r = h.id;
                }
        } else {
        h = e.srcElement;f = h.parentNode;
                        if (f)p = f.id;t = h.id;
        }
        if (t==a || p==a || r==a)return true;
        window.open(document.getElementById(a).href);
        }
}
