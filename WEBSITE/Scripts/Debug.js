const c_DEBUG = true

let DBG
let DBG_PUSH_FMT
let DBG_POP_FMT
{
	let g_DBG_FMT = []
	if (c_DEBUG) {
		DBG = function (...Args) {
			console.log(...g_DBG_FMT, ...Args)
		}

		DBG_PUSH_FMT = function (Format) {
			g_DBG_FMT.push(Format)
		}

		DBG_POP_FMT = function () {
			g_DBG_FMT.pop()
		}
	}
	else {
		DBG = function () { }

		DBG_PUSH_FMT = function () { }

		DBG_POP_FMT = function () { }
	}
}