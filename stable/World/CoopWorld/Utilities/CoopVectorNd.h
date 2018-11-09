//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

#pragma once

#include <vector>
#include <iostream>
#include <iomanip>

#include "CoopPointNd.h"

using namespace std;

// Vector2d is wraps a vector<T> and provides x,y style access
// no error checking is provided for out of range errors
// internally this class uses R(ow) and C(olumn) (i.e. how the data is stored in the data vector)
// the user sees x,y where x = column, y = row

template <typename T> class CoopVector2d {
	vector<T> data;
	int R, C;

	// get index into data vector for a given x,y
	inline int getIndex(int r, int c) {
		return (r * C) + c;
	}

public:
	
	CoopVector2d(){
		R = 0;
		C = 0;
	}
	// construct a vector of size x * y
	CoopVector2d(int x, int y) : R(y), C(x) {
		data.resize(R*C);
	}

	CoopVector2d(int x, int y, T value) : R(y), C(x) {
		data.resize(R*C,value);
	}

	void reset(int x, int y) {
		R = y;
		C = x;
		data.clear();
		data.resize(R*C);
	}

	void reset(int x, int y, T value) {
		R = y;
		C = x;
		data.clear();
		data.resize(R*C, value);
	}

	// overwrite this classes data (vector<T>) with data coppied from newData
	void assign(vector<T> newData) {
		if ((int)newData.size() != R*C) {
			cout << "  ERROR :: in Vector2d::assign() vector provided does not fit. provided vector is size " << newData.size() << " but Rows(" << R << ") * Columns(" << C << ") == " << R*C  << ". Exitting." << endl;
			exit(1);
		}
		data = newData;
	}

	// provides access to value x,y can be l-value or r-value (i.e. used for lookup of assignment)
	T& operator()(int x, int y) {
		return data[getIndex(y, x)];
	}

	T& operator()(double x, double y) {
		return data[getIndex((int)(y), (int)(x))];
	}

	T& operator()(pair<int,int> loc) {
		return data[getIndex(loc.second,loc.first)];
	}

	T& operator()(pair<double,double> loc) {
		return data[getIndex((int)(loc.second),(int)(loc.first))];
	}

	T& operator()(CoopPoint2d loc) {
		return data[getIndex((int)loc.y, (int)loc.x)];
	}

	// show the contents of this Vector2d with index values, and x,y values
	void show() {
		for (int r = 0; r < R; r++) {
			for (int c = 0; c < C; c++) {
				cout << getIndex(r, c) << " : " << c << "," << r << " : " << data[getIndex(r, c)] << endl;
			}
		}
	}

	// show the contents of this Vector2d in a grid
	void showGrid(int precision = -1) {
		if (precision < 0) {
			for (int r = 0; r < R; r++) {
				for (int c = 0; c < C; c++) {
					cout << data[getIndex(r, c)] << " ";
				}
				cout << endl;
			}
		}
		else {
			for (int r = 0; r < R; r++) {
				for (int c = 0; c < C; c++) {
					if (data[getIndex(r, c)] == 0) {
						cout << setfill(' ') << setw((precision * 2) + 2) << " ";
					}
					else {
						cout << setfill(' ') << setw((precision * 2) + 1) << fixed << setprecision(precision) << data[getIndex(r, c)] << " ";
					}
				}
				cout << endl;
			}
		}
	}

	// return raw vector
	vector<T> getRawData() {
		return(data);
	}

	// return a random(ish) locaiont within dist cells of loc.x and loc.y
	// if wrap, then treat grid as tourus, else, stop at edges.
	// pickLeast true = pick low values, false = pick high values
	// method 0 = least in area
	// if more then one cell is least, one will be selected at random
	// method 2 = propotional based on least in area
	// method 3 = random
	CoopPoint2d pickInArea(CoopPoint2d loc, int dist, int method = 0, bool pickLeast = true,  bool includeLoc = false, bool wrap = true) {
		if (dist == 0){
			return { loc };
		}
		if (method == 3) { // pick random... but we must make sure it's a valid pick
			int pickX = Random::getInt(loc.x - dist, loc.x + dist);
			int pickY = Random::getInt(loc.y - dist, loc.y + dist);
			if (!wrap) {
				if (pickX < 0) {
					pickX = 0;
				}
				if (pickX >= C) {
					pickX = C - 1;
				}
				if (pickY < 0) {
					pickY = 0;
				}
				if (pickY >= R) {
					pickY = R - 1;
				}
			}
			else {
				if (pickX < 0) {
					pickX = C + pickX;
				}
				if (pickX >= C) {
					pickX = pickX - C;
				}
				if (pickY < 0) {
					pickY = R + pickY;
				}
				if (pickY >= R) {
					pickY = pickY - R;
				}
			}
			return CoopPoint2d(pickX, pickY);
		}
		else if (method == 0) {
			double least; // current least value or greatest if pickLeast = false
			vector<CoopPoint2d> leastList; // list of cells with least value
			if (includeLoc) { // if passed loc is pickable, then set that as curent least value and add to least list
				least = data[getIndex((int)loc.y, (int)loc.x)];
				leastList.push_back(CoopPoint2d(loc));
			}
			else { // if passed loc is not pickable, then set pick cell to left (if not valid, cell to the right) and set that as curent least value and add to least list
				int x = loc.x - 1;
				int y = loc.y;
				if (x < 0) {
					x = loc.x + 1;
				}
				least = data[getIndex(y, x)];
				leastList.push_back(CoopPoint2d(x, y));
			}
			for (int x = loc.x - dist; x <= loc.x + dist; x++) {
				for (int y = loc.y - dist; y <= loc.y + dist; y++) {
					int realx = x;
					int realy = y;
					if (!wrap) {
						if (realx < 0) {
							realx = 0;
						}
						if (realx >= C) {
							realx = C - 1;
						}
						if (realy < 0) {
							realy = 0;
						}
						if (realy >= R) {
							realy = R - 1;
						}
					}
					else {
						if (realx < 0) {
							realx = C + realx;
						}
						if (realx >= C) {
							realx = realx - C;
						}
						if (realy < 0) {
							realy = R + realy;
						}
						if (realy >= R) {
							realy = realy - R;
						}
					}
					if (pickLeast) {
						if (data[getIndex(realy, realx)] < least) {
							least = data[getIndex(realy, realx)];
							leastList.clear();
							leastList.push_back(CoopPoint2d(realx, realy));
						}
						else if (data[getIndex(realy, realx)] == least) {
							leastList.push_back(CoopPoint2d(realx, realy));
						}
					}
					else {
						if (data[getIndex(realy, realx)] > least) {
							least = data[getIndex(realy, realx)];
							leastList.clear();
							leastList.push_back(CoopPoint2d(realx, realy));
						}
						else if (data[getIndex(realy, realx)] == least) {
							leastList.push_back(CoopPoint2d(realx, realy));
						}

					}
				}
			}
			return leastList[Random::getIndex(leastList.size())];
		}
		else if (method == 1) { // propotional selection
			double minValue, maxValue;
			if (includeLoc) { // if passed loc is pickable, then set that as curent least value and add to least list
				minValue = data[getIndex((int)loc.y, (int)loc.x)];
				maxValue = minValue;
			}
			else { // if passed loc is not pickable, then set pick cell to left (if not valid, cell to the right) and set that as curent least value and add to least list
				int x = loc.x - 1;
				int y = loc.y;
				if (x < 0) {
					x = loc.x + 1;
				}
				minValue = data[getIndex((int)y, (int)x)];
				maxValue = minValue;
			}

			double totalValue = 0; // current least value
			vector<double> values;
			vector<CoopPoint2d> locations;
			for (int x = loc.x - dist; x <= loc.x + dist; x++) {
				for (int y = loc.y - dist; y <= loc.y + dist; y++) {
					int realx = x;
					int realy = y;
					if (!wrap) {
						if (realx < 0) {
							realx = 0;
						}
						if (realx >= C) {
							realx = C - 1;
						}
						if (realy < 0) {
							realy = 0;
						}
						if (realy >= R) {
							realy = R - 1;
						}
					}
					else {
						if (realx < 0) {
							realx = C + realx;
						}
						if (realx >= C) {
							realx = realx - C;
						}
						if (realy < 0) {
							realy = R + realy;
						}
						if (realy >= R) {
							realy = realy - R;
						}
					}
					if (data[getIndex(realy, realx)]< minValue) {
						minValue = data[getIndex(realy, realx)];
					}
					if (data[getIndex(realy, realx)]> maxValue) {
						maxValue = data[getIndex(realy, realx)];
					}
					values.push_back(data[getIndex(realy, realx)]);
					locations.push_back(CoopPoint2d(realx, realy));
				}
			}
			if (pickLeast) {
				for (int i = 0; i < (int)values.size(); i++) {
					values[i] = (maxValue - minValue) - (values[i] - minValue); // translate values so min is max-min and max is 0
					totalValue += values[i];
				}
			}
			else {
				for (int i = 0; i < (int)values.size(); i++) {
					values[i] = (values[i] - minValue); // translate values so max is max-min and min is 0
					totalValue += values[i];
				}
			}
			if (totalValue == 0) { // if all are same value, select random
				//cout << "pickRandom!" << endl;
				return locations[Random::getIndex(locations.size())];
			}
			double pickValue = Random::getDouble(0,totalValue);
			double currValue = values[0];
			int i = 0;
			//cout << "i: " << i << "  " << currValue << " < " << totalValue << endl;
			while (currValue < pickValue) {
				i++;
				currValue += values[i];
				//cout << "i: " << i << "  " << currValue << " < " << totalValue << endl;
			}
			//cout << "out" << endl;
			return CoopPoint2d(locations[i]);
		}
		else {
			cout << "in CoopPoint2d::pickInArea :: method with value " << method << " was passed. But this function requires method to be 0, 1 or 2.\n  exiting." << endl;
			exit(1);
		}
	}

	int x(){
		return C;
	}

	int y(){
		return R;
	}
};

// Vector3d is wraps a vector<T> and provides x,y,z style access
// no error checking is provided for out of range errors
// internally this class uses R(ow), C(olumn) and B(in) (i.e. how the data is stored in the data vector)
// the user sees x,y,z where x = column, y = row and z = bin

template <typename T> class CoopVector3d {
	vector<T> data;
	int R, C, B;

	// get index into data vector for a given x,y,z
	inline int getIndex(int r, int c, int b) {
		return (r*C*B) + (c*B) + b;
	}

public:

	// construct a vector of size x * y * z
	CoopVector3d(int x, int y, int z) : R(y), C(x), B(z) {
		data.resize(C*R*B);
	}

	// overwrite this classes data (vector<T>) with data coppied from newData
	void assign(vector<T> newData, bool byBin = true) {
		if ((int)newData.size() != R*C*B) {
			cout << "  ERROR :: in Vector3d::assign() vector provided does not fit. provided vector is size " << newData.size() << " but Rows(" << R << ") * Columns(" << C << ") * Bins("<<B<<") == " << R*C*B << ". Exitting." << endl;
			exit(1);
		}
		if (byBin) {
			int i = 0;
			for (int b = 0; b < B; b++) {
				for (int r = 0; r < R; r++) {
					for (int c = 0; c < C; c++) {
						cout << i << " " << newData[i] << endl;
						data[getIndex(r,c,b)] = newData[i++];
					}
				}
			}
		} else {
			data = newData;
		}
	}

	// provides access to value x,y,z can be l-value or r-value (i.e. used for lookup of assignment)
	T& operator()(int x, int y, int z) {
		return data[getIndex(y, x, z)]; // i.e. getIndex(r,c,b)
	}

	// returns vector of values for all z at x,y read only!
	vector<T> operator()(int x, int y) {
		vector<T> sub;
		for (int i = getIndex(y, x, 0); i < getIndex(y, x, B); i++) {
			sub.push_back(data[i]);
		}
		return sub;
	}

	// show the contents of this Vector3d with index values, and x,y,z values - used for debuging
	void show() {
		for (int r = 0; r < R; r++) {
			for (int c = 0; c < C; c++) {
				for (int b = 0; b < B; b++) {
					cout << getIndex(r, c, b) << " : " << c << "," << r << "," << b << " : " << data[getIndex(r, c, b)] << endl;
				}
			}
		}
	}

	// show the contents of one x,y for this Vector3d with index z values - used for debuging
	void show(int x, int y) {
		vector<T> sub = (*this)(x, y);
		for (int b = 0; b < B; b++) {
			cout << b << " : " << sub[b] << endl;
		}
	}

	// show the contents for z of this Vector3d in a grid (default z = 0)
	void showGrid(int z = -1) {
		if (z == -1){ // show all
			for (int b = 0; b < B; b++) {
				cout << endl << " bin " << b << " :" << endl;
				showGrid(b);
			}
		}
		else {
			for (int r = 0; r < R; r++) {
				for (int c = 0; c < C; c++) {
					cout << data[getIndex(r, c, z)] << " ";
				}
				cout << endl;
			}
		}
	}

	int x(){
		return C;
	}

	int y(){
		return R;
	}

	int z(){
		return B;
	}

};
