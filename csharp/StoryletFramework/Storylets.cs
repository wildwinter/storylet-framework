/*
 * This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
 * Copyright (c) 2025 Ian Thomas
 */

using ExpressionParser;

namespace StoryletFramework;

public class Storylet
{
    private static readonly Parser expressionParser = new Parser();

    public string Id { get; private set; }
    public dynamic? Content { get; set; }

    // Redraw settings
    public const int REDRAW_ALWAYS = 0;
    public const int REDRAW_NEVER = -1;
    public int Redraw { get; set; } = REDRAW_ALWAYS;

    // Updates to context when drawn
    public Dictionary<string, object>? UpdateOnDrawn { get; set; }

    // Precompiled condition and priority
    private ExpressionNode? _condition;
    private object _priority = 0;

    // The next draw this should be available
    private int _nextDraw = 0;

    public Storylet(string id)
    {
        Id = id;
    }

    // Reset the redraw counter
    public void Reset()
    {
        _nextDraw = 0;
    }

    // Set condition as a precompiled expression
    public void SetCondition(string? text)
    {
        _condition = null;
        if (!string.IsNullOrEmpty(text))
        {
            _condition = expressionParser.Parse(text);
        }
    }

    // Evaluate condition using the current context. Returns true if no condition is set.
    public bool CheckCondition(Dictionary<string, object> context, List<string>? dumpEval = null)
    {
        if (_condition == null)
            return true;

        dumpEval?.Add($"Evaluating condition for {Id}");
        return Convert.ToBoolean(_condition.Evaluate(context, dumpEval));
    }

    // Set priority to a fixed number or a precompiled expression
    public void SetPriority(object numOrExpression)
    {
        if (numOrExpression is int || numOrExpression is double)
        {
            _priority = numOrExpression;
        }
        else if (numOrExpression is string expression)
        {
            _priority = expressionParser.Parse(expression);
        }
        else
        {
            throw new ArgumentException("Priority must be a number or a valid expression.");
        }
    }

    // Evaluate priority using the current context
    public int CalcCurrentPriority(Dictionary<string, object> context, bool useSpecificity = true, List<string>? dumpEval = null)
    {
        int workingPriority = 0;

        if (_priority is int || _priority is double)
        {
            workingPriority = Convert.ToInt32(_priority);
        }
        else if (_priority is ExpressionNode expression)
        {
            dumpEval?.Add($"Evaluating priority for {Id}");
            workingPriority = Convert.ToInt32(expression.Evaluate(context, dumpEval));
        }

        if (useSpecificity)
        {
            workingPriority *= 100;
            if (_condition != null)
            {
                workingPriority += _condition.Specificity;
            }
        }

        return workingPriority;
    }

    // Check if the storylet is available to draw based on redraw rules
    public bool CanDraw(int currentDraw)
    {
        if (Redraw == REDRAW_NEVER && _nextDraw < 0)
            return false;
        if (Redraw == REDRAW_ALWAYS)
            return true;
        return currentDraw >= _nextDraw;
    }

    // Update the redraw counter when the storylet is drawn
    public void Drawn(int currentDraw)
    {
        if (Redraw == REDRAW_NEVER)
        {
            _nextDraw = -1;
        }
        else
        {
            _nextDraw = currentDraw + Redraw;
        }
    }
}

public class Deck
{
    public bool UseSpecificity { get; set; } = false;
    public int AsyncReshuffleCount { get; set; } = 10;

    private readonly Dictionary<string, Storylet> _all = new();
    private readonly List<Storylet> _drawPile = new();
    private int _currentDraw = 0;
    public readonly Dictionary<string, object> Context;

    private ReshuffleState _reshuffleState = new();

    public Deck(Dictionary<string, object>? context = null)
    {
        Context = context ?? new Dictionary<string, object>();
    }

    // Reset the whole deck, including all redraw counters
    public void Reset()
    {
        _currentDraw = 0;
        foreach (var storylet in _all.Values)
        {
            storylet.Reset();
        }
    }

    // Reshuffle the deck
    public void Reshuffle(Func<Storylet, bool>? filter = null, List<string>? dumpEval = null)
    {
        if (AsyncReshuffleInProgress())
            throw new InvalidOperationException("Async reshuffle in progress, can't call Reshuffle().");

        ReshufflePrep(filter, dumpEval);
        ReshuffleDoChunk(_reshuffleState.ToProcess.Count);
        ReshuffleFinalize();
    }

    // Start an async reshuffle
    public void ReshuffleAsync(Action callback, Func<Storylet, bool>? filter = null, List<string>? dumpEval = null)
    {
        if (AsyncReshuffleInProgress())
            throw new InvalidOperationException("Async reshuffle in progress, can't call ReshuffleAsync().");

        ReshufflePrep(filter, dumpEval);
        _reshuffleState.Callback = callback;
    }

    public bool AsyncReshuffleInProgress()
    {
        return _reshuffleState.Callback != null;
    }

    // Update the reshuffle process
    public void Update()
    {
        if (AsyncReshuffleInProgress())
        {
            ReshuffleDoChunk(AsyncReshuffleCount);
            if (_reshuffleState.ToProcess.Count == 0)
            {
                ReshuffleFinalize();
            }
        }
    }

    public Storylet? GetStorylet(string id)
    {
        if (_all.TryGetValue(id, out var storylet))
        {
            return storylet;
        }
        return null;
    }

    public void AddStorylet(Storylet storylet)
    {
        if (_all.ContainsKey(storylet.Id))
            throw new ArgumentException($"Duplicate storylet id: '{storylet.Id}'.");

        _all[storylet.Id] = storylet;
    }

    private void ReshufflePrep(Func<Storylet, bool>? filter, List<string>? dumpEval = null)
    {
        _drawPile.Clear();
        _reshuffleState = new ReshuffleState
        {
            DumpEval = dumpEval,
            Filter = filter,
            PriorityMap = new SortedDictionary<int, List<Storylet>>(Comparer<int>.Create((a, b) => b.CompareTo(a))),
            ToProcess = _all.Values.ToList()
        };
    }

    private void ReshuffleDoChunk(int count)
    {
        var numberToDo = Math.Min(count, _reshuffleState.ToProcess.Count);

        while (numberToDo > 0)
        {
            numberToDo--;

            var storylet = _reshuffleState.ToProcess[0];
            _reshuffleState.ToProcess.RemoveAt(0);

            if (!storylet.CanDraw(_currentDraw))
                continue;

            if (_reshuffleState.Filter != null && !_reshuffleState.Filter(storylet))
                continue;

            if (!storylet.CheckCondition(Context, _reshuffleState.DumpEval))
                continue;

            var priority = storylet.CalcCurrentPriority(Context, UseSpecificity, _reshuffleState.DumpEval);

            if (!_reshuffleState.PriorityMap.ContainsKey(priority))
            {
                _reshuffleState.PriorityMap[priority] = new List<Storylet>();
            }

            _reshuffleState.PriorityMap[priority].Add(storylet);
        }
    }

    private void ReshuffleFinalize()
    {
        foreach (var bucket in _reshuffleState.PriorityMap.Values)
        {
            Utils.ShuffleArray(bucket);
            _drawPile.AddRange(bucket);
        }

        _reshuffleState = new ReshuffleState();
    }

    // Draw the next storylet from the draw pile
    public Storylet? Draw()
    {
        if (AsyncReshuffleInProgress())
            throw new InvalidOperationException("Async reshuffle in progress, can't call Draw().");

        _currentDraw++;

        if (_drawPile.Count == 0)
            return null;

        var storylet = _drawPile[0];
        _drawPile.RemoveAt(0);

        if (storylet.UpdateOnDrawn != null)
        {
            ContextUtils.UpdateContext(Context, storylet.UpdateOnDrawn);
        }

        storylet.Drawn(_currentDraw);
        return storylet;
    }

    public List<Storylet> DrawHand(int count, bool reshuffleIfNeeded = false)
    {
        var storylets = new List<Storylet>();
    
        for (int i = 0; i < count; i++)
        {
            if (_drawPile.Count == 0)
            {
                if (reshuffleIfNeeded)
                {
                    Reshuffle();
                }
                else
                {
                    break;
                }
            }
    
            var storylet = Draw();
            if (storylet == null)
            {
                break;
            }
    
            storylets.Add(storylet);
        }
    
        return storylets;
    }

    // For debugging: Dump the IDs of the current draw pile
    public string DumpDrawPile()
    {
        if (AsyncReshuffleInProgress())
            throw new InvalidOperationException("Async reshuffle in progress, can't call DumpDrawPile().");

        return string.Join(",", _drawPile.Select(s => s.Id));
    }

    private class ReshuffleState
    {
        public Action? Callback { get; set; }
        public List<Storylet> ToProcess { get; set; } = new();
        public Func<Storylet, bool>? Filter { get; set; }
        public SortedDictionary<int, List<Storylet>> PriorityMap { get; set; } = new();
        public List<string>? DumpEval { get; set; }
    }
}
