
#ifndef _DOMAIN_H_
#define _DOMAIN_H_

#include "AgentAction.h"
#include "Task.h"
#include "HTNMethod.h"
#include "HTNOperator.h"
#include "TemporalAction.h"
#include "And.h"
#include "Derived.h"
#include "Equals.h"
#include "Exists.h"
#include "Forall.h"
#include "Function.h"
#include "GroundFunc.h"
#include "Increase.h"
#include "NetworkNode.h"
#include "Not.h"
#include "Oneof.h"
#include "Or.h"
#include "EitherType.h"
#include "When.h"

#define DOMAIN_DEBUG false

// union-find: return the root in the tree that n belongs to
inline unsigned uf( UnsignedVec & mf, unsigned n ) {
	if ( mf[n] == n ) return n;
	else return mf[n] = uf( mf, mf[n] );
}

class Domain {

public:

	std::string name;                   // name of domain

	bool equality;                      // whether domain supports equality
	bool strips, adl, condeffects;      // whether domain is STRIPS, ADL and/or has conditional effects
	bool typed, cons, costs;            // whether domain is typed, has constants, has costs
	bool temp, nondet;                  // whether domain is temporal, is non-deterministic
	bool multiagent, unfact, fact, net, shop; // whether domain is multiagent and unfactored/factored/networked, shop

	TokenStruct< Type * > types;        // types
	TokenStruct< Lifted * > preds;      // predicates
	TokenStruct< Function * > funcs;    // functions
	TokenStruct< Action * > actions;    // actions
	TokenStruct< HTNMethod * > htnMtds;// shop methods
	TokenStruct< Derived * > derived;   // derived predicates
	TokenStruct< HTNOperator * > htnOps;// shop operators
	TokenStruct< Task * > tasks;		// tasks

	TokenStruct< NetworkNode * > nodes; // nodes of concurrency network
	PairVec edges;                      // edges of concurrency network
	UnsignedVec mf;                     // merge-find for connected components

	Domain()
		: equality( false ), strips( false ), adl( false ), condeffects( false )
		, typed( false ), cons( false ), costs( false ), temp( false ), nondet( false )
		, multiagent( false ), unfact( false ), fact( false ), net( false ), shop( false ) {
		types.insert( new Type( "OBJECT" ) );
	}

	Domain( const std::string & s, bool htn = false )
		: equality( false ), strips( false ), adl( false ), condeffects( false )
		, typed( false ), cons( false ), costs( false ), temp( false ), nondet( false )
		, multiagent( false ), unfact( false ), fact( false ), net( false ), shop( htn ) {

		// Type 0 is always "OBJECT", whether the domain is typed or not
		types.insert( new Type( "OBJECT" ) );

		if(shop) parseSHOP( s );
		else     parse( s );
	}

	~Domain() {
		for ( unsigned i = 0; i < types.size(); ++i )
			delete types[i];
		for ( unsigned i = 0; i < preds.size(); ++i )
			delete preds[i];
		for ( unsigned i = 0; i < funcs.size(); ++i )
			delete funcs[i];
		for ( unsigned i = 0; i < actions.size(); ++i )
			delete actions[i];
		for ( unsigned i = 0; i < derived.size(); ++i )
			delete derived[i];
		for ( unsigned i = 0; i < nodes.size(); ++i )
			delete nodes[i];
		for ( unsigned i = 0; i < tasks.size(); ++i )
		 	delete tasks[i];
		for ( unsigned i = 0; i < htnMtds.size(); ++i )
			delete htnMtds[i];
		for ( unsigned i = 0; i < htnOps.size(); ++i )
			delete htnOps[i];
	}

	void parse( const std::string & s ) {
		Filereader f( s );
		name = f.parseName( "DOMAIN" );

		if ( DOMAIN_DEBUG ) std::cout << name << "\n";

		for ( ; f.getChar() != ')'; f.next() ) {
			f.assert( "(" );
			f.assert( ":" );
			std::string t = f.getToken();

			if ( DOMAIN_DEBUG ) std::cout << t << "\n";

			if ( t == "REQUIREMENTS" ) parseReq( f );
			else if ( t == "TYPES" ) parseTypes( f );
			else if ( t == "CONSTANTS" ) parseConstants( f );
			else if ( t == "PREDICATES" ) parsePredicates( f );
			else if ( t == "FUNCTIONS" ) parseFunctions( f );
			else if ( t == "ACTION" ) parseAction( f );
			else if ( t == "DURATIVE-ACTION" ) parseDurativeAction( f );
			else if ( t == "CONCURRENCY-CONSTRAINT" ) parseNetworkNode( f );
			else if ( t == "POSITIVE-DEPENDENCE" ) parseNetworkEdge( f );
			else if ( t == "DERIVED" ) parseDerived( f );
//			else if ( t == "AXIOM" ) parseAxiom( f );
			else f.tokenExit( t );
		}
	}

	void parseSHOP( const std::string & s ) {
		Filereader f( s );

	    name = f.parseHTNDomainName( );

		if ( DOMAIN_DEBUG ) std::cout << name << "\n";

		for ( ; f.getChar() != ')'; f.next() ) {
			f.assert( "(" );
			f.assert( ":" );
			std::string t = f.getToken();

			if ( DOMAIN_DEBUG ) std::cout << t << "\n";

			if ( t == "OPERATOR" ) parseOperator( f );
		    else if ( t == "METHOD" ) parseMethod( f );
			else f.tokenExit( t );
		}
	}

	void parseReq( Filereader & f ) {
		for ( f.next(); f.getChar() != ')'; f.next() ) {
			f.assert( ":" );
			std::string s = f.getToken();

			if ( DOMAIN_DEBUG ) std::cout << "  " << s << "\n";

			if ( s == "STRIPS" ) strips = true;
			else if ( s == "ADL" ) adl = true;
			else if ( s == "CONDITIONAL-EFFECTS" ) condeffects = true;
			else if ( s == "TYPING" ) typed = true;
			else if ( s == "ACTION-COSTS" ) costs = true;
			else if ( s == "EQUALITY" ) equality = true;
			else if ( s == "DURATIVE-ACTIONS" ) temp = true;
			else if ( s == "NON-DETERMINISTIC" ) nondet = true;
			else if ( s == "MULTI-AGENT" ) multiagent = true;
			else if ( s == "UNFACTORED-PRIVACY" ) unfact = true;
			else if ( s == "FACTORED-PRIVACY" ) fact = true;
			else if ( s == "CONCURRENCY-NETWORK" ) net = true;

			else f.tokenExit( s );
		}

		++f.c;
	}

	// get the type corresponding to a string
	Type * getType( std::string s ) {
		int i = types.index( s );
		if ( i < 0 ) {
			if ( s[0] == '(' ) {
				i = types.insert( new EitherType( s ) );
				for ( unsigned k = 9; s[k] != ')'; ) {
					unsigned e = s.find( ' ', k );
					types[i]->subtypes.push_back( getType( s.substr( k, e - k ) ) );
					k = e + 1;
				}
			}
			else i = types.insert( new Type( s ) );
		}
		return types[i];
	}

	// convert a vector of type names to integers
	IntVec convertTypes( const StringVec & v ) {
		IntVec out;
		for ( unsigned i = 0; i < v.size(); ++i )
			out.push_back( types.index( getType( v[i] )->name ) );
		return out;
	}

	void parseTypes( Filereader & f ) {
		if ( !typed ) {
			std::cout << "Requirement :TYPING needed to define types\n";
			exit( 1 );
		}

//		if this makes it in, probably need to define new subclass of Type
//		if ( costs ) insert( new Type( "NUMBER" ), tmap, types );

		// Parse the typed list
		TokenStruct< std::string > ts = f.parseTypedList( false );

		// bit of a hack to avoid OBJECT being the supertype
		if ( ts.index( "OBJECT" ) >= 0 ) {
			types[0]->name = "SUPERTYPE";
			types.tokenMap.clear();
			types.tokenMap["SUPERTYPE"] = 0;
		}

		// Relate subtypes and supertypes
		for ( unsigned i = 0; i < ts.size(); ++i ) {
			if ( ts.types[i].size() )
				getType( ts.types[i] )->insertSubtype( getType( ts[i] ) );
			else getType( ts[i] );
		}

		// By default, the supertype of a type is "OBJECT"
		for ( unsigned i = 1; i < types.size(); ++i )
			if ( types[i]->supertype == 0 )
				types[0]->insertSubtype( types[i] );

		for ( unsigned i = 0; DOMAIN_DEBUG && i < types.size(); ++i )
			std::cout << "  " << types[i];
	}

	void parseConstants( Filereader & f ) {
		if ( typed && !types.size() ) {
			std::cout << "Types needed before defining constants\n";
			exit( 1 );
		}

		cons = true;

		TokenStruct< std::string > ts = f.parseTypedList( true, types );

		for ( unsigned i = 0; i < ts.size(); ++i )
			getType( ts.types[i] )->constants.insert( ts[i] );

		for ( unsigned i = 0; DOMAIN_DEBUG && i < types.size(); ++i ) {
			std::cout << " ";
			if ( typed ) std::cout << " " << types[i] << ":";
			for ( unsigned j = 0; j < types[i]->constants.size(); ++j )
				std::cout << " " << types[i]->constants[j];
			std::cout << "\n";
		}
	}

	void parsePredicates( Filereader & f ) {
		if ( typed && !types.size() ) {
			std::cout << "Types needed before defining predicates\n";
			exit(1);
		}

		for ( f.next(); f.getChar() != ')'; f.next() ) {
			f.assert( "(" );
			if ( f.getChar() == ':' ) {
				// Needed to support MA-PDDL
				f.assert( ":PRIVATE" );
				f.parseTypedList( true, types, "(" );

				// CURRENT HACK: TOTALLY IGNORE PRIVATE !!!
				--f.c;
				parsePredicates( f );
			}
			else {
				Lifted * c = new Lifted( f.getToken() );
				c->parse( f, types[0]->constants, *this );

				if ( DOMAIN_DEBUG ) std::cout << "  " << c;
				preds.insert( c );
			}
		}
		++f.c;
	}

	void parseFunctions( Filereader & f ) {
		if ( typed && !types.size() ) {
			std::cout << "Types needed before defining functions\n";
			exit(1);
		}

		for ( f.next(); f.getChar() != ')'; f.next() ) {
			f.assert( "(" );
			Function * c = new Function( f.getToken() );
			c->parse( f, types[0]->constants, *this );

			if ( DOMAIN_DEBUG ) std::cout << "  " << c;
			funcs.insert( c );
		}
		++f.c;
	}

	void parseAction( Filereader & f ) {
		if ( !preds.size() ) {
			std::cout << "Predicates needed before defining actions\n";
			exit(1);
		}

		f.next();
		Action * a = 0;

		// If domain is multiagent, parse using AgentAction
		if ( multiagent ) a = new AgentAction( f.getToken() );
		else a = new Action( f.getToken() );

		a->parse( f, types[0]->constants, *this );

		if ( DOMAIN_DEBUG ) std::cout << a << "\n";
		actions.insert( a );
	}

	void parseDerived( Filereader & f ) {
		if ( !preds.size() ) {
			std::cout << "Predicates needed before defining derived predicates\n";
			exit(1);
		}

		f.next();
		Derived * d = new Derived;
		d->parse( f, types[0]->constants, *this );

		if ( DOMAIN_DEBUG ) std::cout << d << "\n";
		derived.insert( d );
	}

	void parseDurativeAction( Filereader & f ) {
		if ( !preds.size() ) {
			std::cout << "Predicates needed before defining actions\n";
			exit(1);
		}

		f.next();
		Action * a = new TemporalAction( f.getToken() );
		a->parse( f, types[0]->constants, *this );

		if ( DOMAIN_DEBUG ) std::cout << a << "\n";
		actions.insert( a );
	}

	void parseNetworkNode( Filereader & f ) {
		if ( !preds.size() || !actions.size() ) {
			std::cout << "Predicates and actions needed before defining a concurrency network\n";
			exit(1);
		}

		f.next();
		NetworkNode * n = new NetworkNode( f.getToken() );
		n->parse( f, types[0]->constants, *this );

		if ( DOMAIN_DEBUG ) std::cout << n << "\n";

		nodes.insert( n );
		mf.push_back( mf.size() );
	}

	void parseNetworkEdge( Filereader & f ) {
		f.next();
		int n1 = nodes.index( f.getToken( nodes ) );
		f.next();
		int n2 = nodes.index( f.getToken( nodes ) );
		edges.push_back( std::make_pair( n1, n2 ) );

		unsigned a = uf( mf, n1 ), b = uf( mf, n2 );
		if ( a != b ) mf[MIN( a, b )] = MAX( a, b );

		f.next();
		f.assert( ")" );
	}

	void parseOperator( Filereader & f ) {
		f.next();
		f.assert( "(" );
		HTNOperator * o = 0;
		o = new HTNOperator( f.getToken() );
		o->SHOPparse( f, types[0]->constants, *this );

		if ( DOMAIN_DEBUG ) std::cout << o << "\n";
		htnOps.insert( o );
	}

	void parseMethod( Filereader & f ) {
		f.next();
		f.assert( "(" );
		HTNMethod * o = 0;
		o = new HTNMethod( f.getToken() );
		o->SHOPparse( f, types[0]->constants, *this );

		if ( DOMAIN_DEBUG ) std::cout << o << "\n";
		htnMtds.insert( o );
	}

	// Return a copy of the type structure, with newly allocated types
	// This will also copy all constants and objects!
	TokenStruct< Type * > copyTypes() {
		TokenStruct< Type * > out;
		for ( unsigned i = 0; i < types.size(); ++i )
			out.insert( types[i]->copy() );

		for ( unsigned i = 1; i < types.size(); ++i ) {
			if ( types[i]->supertype )
				out[out.index( types[i]->supertype->name )]->insertSubtype( out[i] );
			else
				out[i]->copySubtypes( types[i], out );
		}

		return out;
	}

	// Set the types to "otherTypes"
	void setTypes( const TokenStruct< Type * > & otherTypes ) {
		for ( unsigned i = 0; i < types.size(); ++i )
			delete types[i];
		types = otherTypes;
	}

	// Create a type with a given supertype (default is "OBJECT")
	void createType( const std::string & name, const std::string & parent = "OBJECT" ) {
		Type * type = new Type( name );
		types.insert( type );
		types.get( parent )->insertSubtype( type );
	}

	// Create a constant of a given type
	void createConstant( const std::string & name, const std::string & type ) {
		types.get( type )->constants.insert( name );
	}

	// Create a predicate with the given name and parameter types
	Lifted * createPredicate( const std::string & name, const StringVec & params = StringVec() ) {
		Lifted * pred = new Lifted( name );
		for ( unsigned i = 0; i < params.size(); ++i )
			pred->params.push_back( types.index( params[i] ) );
		preds.insert( pred );
		return pred;
	}

	// Create a function with the given name and parameter types
	Lifted * createFunction( const std::string & name, int type, const StringVec & params = StringVec() ) {
		Function * func = new Function( name, type );
		for ( unsigned i = 0; i < params.size(); ++i )
			func->params.push_back( types.index( params[i] ) );
		funcs.insert( func );
		return func;
	}

	// Create an action with the given name and parameter types
	Action * createAction( const std::string & name, const StringVec & params = StringVec() ) {
		Action * action = new Action( name );
		for ( unsigned i = 0; i < params.size(); ++i )
			action->params.push_back( types.index( params[i] ) );
		actions.insert( action );
		return action;
	}

	// Set the precondition of an action to "cond", converting to "And"
	void setPre( const std::string & act, Condition * cond ) {
		Action * action = actions.get( act );

		And * old = dynamic_cast< And * >( cond );
		if ( old == 0 ) {
			action->pre = new And;
			if ( cond ) dynamic_cast< And * >( action->pre )->add( cond->copy( *this ) );
		}
		else action->pre = old->copy( *this );
	}

	// Add a precondition to the action with name "act"
	void addPre( bool neg, const std::string & act, const std::string & pred, const IntVec & params = IntVec() ) {
		Action * action = actions.get( act );
		if ( action->pre == 0 ) action->pre = new And;
		And * a = dynamic_cast< And * >( action->pre );
		if ( neg ) a->add( new Not( ground( pred, params ) ) );
		else a->add( ground( pred, params ) );
	}

	// Add an "OR" precondition to the action with name "act"
	void addOrPre( const std::string & act, const std::string & pred1, const std::string & pred2,
	               const IntVec & params1 = IntVec(), const IntVec & params2 = IntVec() ) {
		Or * o = new Or;
		o->first = ground( pred1, params1 );
		o->second = ground( pred2, params2 );
		Action * action = actions.get( act );
		And * a = dynamic_cast< And * >( action->pre );
		a->add( o );
	}

	// Set the precondition of an action to "cond", converting to "And"
	void setEff( const std::string & act, Condition * cond ) {
		Action * action = actions.get( act );

		And * old = dynamic_cast< And * >( cond );
		if ( old == 0 ) {
			action->eff = new And;
			if ( cond ) dynamic_cast< And * >( action->eff )->add( cond->copy( *this ) );
		}
		else action->eff = old->copy( *this );
	}

	// Add an effect to the action with name "act"
	void addEff( bool neg, const std::string & act, const std::string & pred, const IntVec & params = IntVec() ) {
		Action * action = actions.get( act );
		if ( action->eff == 0 ) action->eff = new And;
		And * a = dynamic_cast< And * >( action->eff );
		if ( neg ) a->add( new Not( ground( pred, params ) ) );
		else a->add( ground( pred, params ) );
	}

	// Add a cost to the action with name "act", in the form of an integer
	void addCost( const std::string & act, int cost ) {
		Action * action = actions.get( act );
		if ( action->eff == 0 ) action->eff = new And;
		And * a = dynamic_cast< And * >( action->eff );
		a->add( new Increase( cost ) );
	}

	// Add a cost to the action with name "act", in the form of a function
	void addCost( const std::string & act, const std::string & func, const IntVec & params = IntVec() ) {
		Action * action = actions.get( act );
		if ( action->eff == 0 ) action->eff = new And;
		And * a = dynamic_cast< And * >( action->eff );
		a->add( new Increase( funcs.get( func ), params ) );
	}

	// Create a ground condition with the given name
	Ground * ground( const std::string & name, const IntVec & params = IntVec() ) {
		return new Ground( preds[preds.index( name )], params );
	}

	// Return the list of type names corresponding to a parameter list
	StringVec typeList( ParamCond * c ) {
		StringVec out;
		for ( unsigned i = 0; i < c->params.size(); ++i )
			out.push_back( types[c->params[i]]->name );
		return out;
	}

	// Return the list of object names corresponding to a ground fluent
	StringVec objectList( TypeGround * g ) {
		StringVec out;
		for ( unsigned i = 0; i < g->params.size(); ++i )
			out.push_back( types[g->lifted->params[i]]->object( g->params[i] ).first );
		return out;
	}

	// Add parameters to an action
	void addParams( const std::string & name, const StringVec & v ) {
		actions.get( name )->addParams( convertTypes( v ) );
	}

	// Assert that one type is a subtype of another
	bool assertSubtype( int t1, int t2 ) {
		for ( Type * type = types[t1]; type != 0; type = type->supertype )
			if ( type->name == types[t2]->name ) return 1;
		return 0;
	}

	// return the index of a constant for a given type
	int constantIndex( const std::string & name, const std::string & type ) {
		return types.get( type )->parseConstant( name ).second;
	}

	// Print the domain in PDDL format
	void PDDLPrint( std::ostream & stream ) {
		stream << "( DEFINE ( DOMAIN " << name << " )\n";
		stream << "( :REQUIREMENTS";
		if ( equality ) stream << " :EQUALITY";
		if ( strips ) stream << " :STRIPS";
		if ( costs ) stream << " :ACTION-COSTS";
		if ( adl ) stream << " :ADL";
		if ( condeffects ) stream << " :CONDITIONAL-EFFECTS";
		if ( typed ) stream << " :TYPING";
		if ( temp ) stream << " :DURATIVE-ACTIONS";
		if ( nondet ) stream << " :NON-DETERMINISTIC";
		if ( multiagent ) stream << " :MULTI-AGENT";
		if ( unfact ) stream << " :UNFACTORED-PRIVACY";
		if ( fact ) stream << " :FACTORED-PRIVACY";
		if ( net ) stream << " :CONCURRENCY-NETWORK";
		stream << " )\n";

		if ( typed ) {
			stream << "( :TYPES\n";
			for ( unsigned i = 1; i < types.size(); ++i )
				types[i]->PDDLPrint( stream );
			stream << ")\n";
		}

		if ( cons ) {
			stream << "( :CONSTANTS\n";
			for ( unsigned i = 0; i < types.size(); ++i )
				if ( types[i]->constants.size() ) {
					stream << "\t";
					for ( unsigned j = 0; j < types[i]->constants.size(); ++j )
						stream << types[i]->constants[j] << " ";
					if ( typed )
						stream << "- " << types[i]->name;
					stream << "\n";
				}
			stream << ")\n";
		}

		stream << "( :PREDICATES\n";
		for ( unsigned i = 0; i < preds.size(); ++i ) {
			preds[i]->PDDLPrint( stream, 1, TokenStruct< std::string >(), *this );
			stream << "\n";
		}
		stream << ")\n";

		if ( funcs.size() ) {
			stream << "( :FUNCTIONS\n";
			for ( unsigned i = 0; i < funcs.size(); ++i ) {
				funcs[i]->PDDLPrint( stream, 1, TokenStruct< std::string >(), *this );
				stream << "\n";
			}
			stream << ")\n";
		}

		for ( unsigned i = 0; i < actions.size(); ++i )
			actions[i]->PDDLPrint( stream, 0, TokenStruct< std::string >(), *this );

		for ( unsigned i = 0; i < derived.size(); ++i )
			derived[i]->PDDLPrint( stream, 0, TokenStruct< std::string >(), *this );

		for ( unsigned i = 0; i < nodes.size(); ++i )
			nodes[i]->PDDLPrint( stream, 0, TokenStruct< std::string >(), *this );

		for ( unsigned i = 0; i < edges.size(); ++i ) {
			stream << "( :POSITIVE-DEPENDENCE ";
			stream << nodes[edges[i].first]->name << " ";
			stream << nodes[edges[i].second]->name << " )\n";
		}

		stream << ")\n";
	}

	// Print the domain in SHOP format
	void SHOPPrint( std::ostream & stream ) {
		stream << "( DEFDOMAIN " << name << " (\n";		

		 for ( unsigned i = 0; i < htnOps.size(); ++i ){
		 		htnOps[i]->SHOPPrint( stream, 0, TokenStruct< std::string >(), *this );
		 }

		 for ( unsigned i = 0; i < htnMtds.size(); ++i )
		 		htnMtds[i]->SHOPPrint( stream, 0, TokenStruct< std::string >(), *this );

		stream << ")\n";

	}

};

#endif
