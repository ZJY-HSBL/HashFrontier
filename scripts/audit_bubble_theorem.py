#!/usr/bin/env python3
"""Necessary numeric preconditions: NEVER a theorem proof."""
import argparse,json,math
from pathlib import Path
def audit(n,epsilon,alpha,d_core,implemented_d):
    if n<=1 or not(0<epsilon<1) or not(0<alpha<1) or d_core<2 or implemented_d<2:raise ValueError("invalid paper parameters")
    minimum=n**(-.25);maximum=math.exp(-d_core);d=math.ceil(math.log(1/epsilon)+alpha)
    necessary={"epsilon_ge_n_minus_quarter":epsilon>=minimum,"epsilon_le_exp_minus_d_core":epsilon<=maximum,
       "hash_choice_count":implemented_d==d,"core_fits_choices":implemented_d>=d_core}
    return {"n":n,"epsilon":epsilon,"alpha":alpha,"d_core":d_core,"implemented_d":implemented_d,
       "theorem_d":d,"min_epsilon":minimum,"max_epsilon":maximum,"necessary_conditions":necessary,
       "all_numeric_necessary":all(necessary.values()),"paper_randomness_model":"fully_independent_random_hash_functions",
       "implementation_randomness_model":"deterministic_splitmix_derived_not_fully_independent",
       "sufficiently_large_core_constant_verified":False,"eligible_to_claim_theorem":False,
       "note":"Numerical conditions are only necessary. Hash independence, large constants and algorithm mapping remain unverified."}
def main():
    p=argparse.ArgumentParser()
    p.add_argument("--n",type=int,default=4096);p.add_argument("--alpha",type=float,default=.5)
    p.add_argument("--d-core",type=int,default=5);p.add_argument("--d",type=int,default=10)
    p.add_argument("--loads",default="70,85,95");p.add_argument("--out",default="results/v0.10.0-bubble-theorem-audit.json")
    a=p.parse_args()
    rows=[audit(a.n,1-int(x)/100,a.alpha,a.d_core,a.d) for x in a.loads.split(",")]
    output={"paper":"SODA 2025 Bubble-up, Theorem 5.1 / Corollary 3.1","results":rows}
    Path(a.out).parent.mkdir(parents=True,exist_ok=True);Path(a.out).write_text(json.dumps(output,indent=2)+"\n")
    print(json.dumps({str(round((1-r["epsilon"])*100)):r["necessary_conditions"] for r in rows},indent=2))
if __name__=="__main__":main()
