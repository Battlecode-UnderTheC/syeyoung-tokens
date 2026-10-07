import json,os,sys,collections
sys.path.insert(0,'.')
import decode
rows={r['id']:r for r in json.load(open('../data/rows.json'))}
files=[('../data/rep/'+f,int(f.split('.')[0])) for f in sorted(os.listdir('../data/rep'))]
part=int(sys.argv[1]); files=files[part::2]
out=[]
for path,i in files:
    r=rows[i]; us=r['us']
    if r['rounds']<380: continue
    d=decode.load(path)
    C=collections.Counter(); cur=None
    for kind,st,info in decode.walk(d):
        if kind=='pre': cur=info
        if kind=='death' and info[0]>=360 and info[3]=='head_to_head':
            rnd,k,team,reason,length,detail=info
            side='us' if team==us else 'th'
            moverteam=st.team[cur]
            init='self-moved' if detail=='mover' else ('lunged' if detail.endswith('lunge') else 'stepped-on' if 'enemy' in detail else 'ally')
            ini_side = 'init by us' if moverteam==us else 'init by them'
            C[f"{side} {ini_side} {'L>=8' if length>=8 else 'L<8'}"]+=length
            C[f"{side} {ini_side} {'L>=8' if length>=8 else 'L<8'} n"]+=1
    out.append(dict(id=i,res=r['res'],reason=r['reason'],C=dict(C)))
json.dump(out,open(f'../data/headon{part}.json','w'))
