function SetEnabled(Item, Value) {
	if (!Value)
		Item.attr("disabled", "true")
	else
		Item.removeAttr("disabled")
}

function CreateElement(TagName, Parent, Attributes, InnerHTML) {
	let NewElement = document.createElement(TagName)

	if (typeof (Attributes) != "undefined") {
		for (let Key in Attributes) {
			NewElement.setAttribute(Key, Attributes[Key])
		}
	}

	NewElement.innerHTML = typeof (InnerHTML) != "undefined" ? InnerHTML : ""
	if (Parent) { Parent.append(NewElement) }

	return NewElement
}

function Round(Value, Precision) {
	return Math.round(Value * Math.pow(10, Precision)) / Math.pow(10, Precision)
}

function MeasureData(Value, Precision = 2) {
	const Measurements = [
		"B",
		"KB",
		"MB",
		"GB",
		"TB",
		"EB",
		"ZB",
		"YB",
		"RB",
		"QB",
		"UNK"
	]

	let Iteration = 0
	while (Value >= 1024) {
		Value /= 1024
		Iteration++
	}


	let Type = ""
	if (Iteration > Measurements.length)
		Type = Measurements[ListSize - 1]
	else
		Type = Measurements[Iteration]

	return `${Round(Value, Precision)}${Type}`
}

