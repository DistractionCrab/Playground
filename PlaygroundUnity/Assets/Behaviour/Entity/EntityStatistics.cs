using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using UnityEngine;
using UnityEditor;

[System.Serializable]
public class EntityStatistics : MonoBehaviour
{
	#region Variables
	private int _maxHealth;	
    private int _minHealth;
        #region HealthVariables
        [SerializeField]
	    private int _health;
	    private Action<EntityStatistics, int> _onHealthDiff = (EntityStatistics a, int b) => { };
        private readonly List<Func<EntityStatistics, int, int>> _preHealthDiff = new();
	    #endregion
	#endregion


	void Start()
    {

    }


    public void ApplyDamage(int dmg)
    {
        dmg = this._preHealthDiff.Aggregate(
            0, 
            (int acc, Func<EntityStatistics, int, int> f) => { 
                return acc + f(this, dmg); 
            });
        this._health += dmg;
		this._onHealthDiff(this, dmg);
	}


    public int Health {
        get { return this._health; }
        set { this._health = value; }
    }

    public Action<EntityStatistics, int> OnHealthDiff {
        get { return this._onHealthDiff; }
        set { this._onHealthDiff = value; }
    }

    public List<Func<EntityStatistics, int, int>> PreHealthDiff
    {
        get { return this._preHealthDiff; }
    }
}
